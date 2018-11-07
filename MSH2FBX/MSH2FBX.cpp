#include "pch.h"
#include "MSH2FBX.h"
#include "CLI11.hpp"
#include "Converter.h"
#include <math.h>
#include <filesystem>

namespace MSH2FBX
{
	namespace fs = std::filesystem;

	void Log(const char* msg)
	{
		std::cout << msg << std::endl;
	}

	void Log(const string& msg)
	{
		std::cout << msg << std::endl;
	}

	void LogEntry(LoggerEntry entry)
	{
		if (entry.m_Level >= ELogType::Warning)
		{
			Log(entry.m_Message.c_str());
		}
	}

	function<void(LoggerEntry)> LogCallback = LogEntry;

	string GetFileName(const string& path)
	{
		char sep = '\\';
		size_t i = path.rfind(sep, path.length());
		if (i != string::npos) {
			return(path.substr(i + 1, path.length() - i));
		}
		return("");
	}

	string RemoveFileExtension(const string& fileName)
	{
		size_t lastindex = fileName.find_last_of(".");

		if (lastindex == string::npos)
		{
			return fileName;
		}

		return fileName.substr(0, lastindex);
	}

	vector<string> GetFiles(string Directory, string Extension, bool recursive)
	{
		vector<string> mshFiles;
		if (!fs::exists(Directory))
		{
			return mshFiles;
		}

		for (auto& p : fs::directory_iterator(Directory))
		{
			if (p.path().extension() == Extension)
			{
				mshFiles.insert(mshFiles.end(), p.path().string());
			}

			if (recursive && p.is_directory())
			{
				vector<string>& next = GetFiles(p.path().string(), Extension, recursive);
				mshFiles.insert(mshFiles.end(), next.begin(), next.end());
			}
		}
		return mshFiles;
	}

	vector<string> GetFiles(vector<string> Paths, string Extension, bool recursive)
	{
		vector<string> files;
		for (auto& it = Paths.begin(); it != Paths.end(); ++it)
		{
			auto p = fs::path(*it);

			if (fs::exists(p))
			{
				if (fs::is_directory(p))
				{
					vector<string>& next = GetFiles(*it, Extension, recursive);
					files.insert(files.end(), next.begin(), next.end());
				}
				else if (fs::is_regular_file(p))
				{
					files.insert(files.end(), *it);
				}
			}
		}
		return files;
	}
}

// the main function must not lie inside a namespace
int main(int argc, char *argv[])
{
	using std::string;
	using std::vector;
	using std::map;
	using LibSWBF2::Logging::Logger;
	using LibSWBF2::Chunks::Mesh::MSH;
	using MSH2FBX::Converter;
	using MSH2FBX::EChunkFilter;
	using MSH2FBX::EModelPurpose;

	Logger::SetLogCallback(MSH2FBX::LogCallback);

	vector<string> files;
	vector<string> animations;
	vector<string> models;
	map<string, EModelPurpose> filterMap
	{
		// Meshes
		{"Mesh", EModelPurpose::Mesh},
		{"Mesh_Regular", EModelPurpose::Mesh_Regular},
		{"Mesh_Lowrez", EModelPurpose::Mesh_Lowrez},
		{"Mesh_Collision", EModelPurpose::Mesh_Collision},
		{"Mesh_VehicleCollision", EModelPurpose::Mesh_VehicleCollision},
		{"Mesh_ShadowVolume", EModelPurpose::Mesh_ShadowVolume},
		{"Mesh_TerrainCut", EModelPurpose::Mesh_TerrainCut},

		// Just Points
		{"Point", EModelPurpose::Point},
		{"Point_EmptyTransform", EModelPurpose::Point_EmptyTransform},
		{"Point_DummyRoot", EModelPurpose::Point_DummyRoot},
		{"Point_HardPoint", EModelPurpose::Point_HardPoint},

		// Skeleton
		{"Skeleton", EModelPurpose::Skeleton},
		{"Skeleton_Root", EModelPurpose::Skeleton_Root},
		{"Skeleton_BoneRoot", EModelPurpose::Skeleton_BoneRoot},
		{"Skeleton_BoneLimb", EModelPurpose::Skeleton_BoneLimb},
		{"Skeleton_BoneEnd", EModelPurpose::Skeleton_BoneEnd},
	};
	vector<string> filter;
	string fbxFileName = "";

	// Build up Command Line Parser
	CLI::App app
	{
		"--------------------------------------------------------------\n"
		"-------------------- MSH to FBX Converter --------------------\n"
		"--------------------------------------------------------------\n"
		"Web: https://github.com/Ben1138/MSH2FBX \n"
	};

	CLI::Option* filesOpt = app.add_option("-f,--files", files, "MSH file paths (file or directory, one or more), importing everything")->check(CLI::ExistingPath);
	CLI::Option* animsOpt = app.add_option("-a,--animations", animations, "MSH file Paths (file or directory, one or more), importing Animation Data only")->check(CLI::ExistingPath);
	CLI::Option* modelsOpt = app.add_option("-m,--models", models, "MSH file paths (file or directory, one or more), importing Model Data only")->check(CLI::ExistingPath);
	CLI::Option* nameOpt = app.add_option("-n,--name", fbxFileName, "Name of the resulting FBX File (optional)");
	CLI::Option* overOpt = app.add_flag("-o,--override-anim-name", "Will use the MSH file name as Animation name rather than the Animation name stored inside the MSH file");
	CLI::Option* recOpt = app.add_flag("-r,--recursive", "For all given directories, crawling will be recursive (will include all sub-directories)");

	string filterOptionInfo = "What to ignore. Options are:\n";
	for (auto& it = filterMap.begin(); it != filterMap.end(); ++it)
	{
		filterOptionInfo += "\t\t\t\t" + it->first + "\n";
	}
	app.add_option("-i,--ignore", filter, filterOptionInfo);

	// *parse magic*
	CLI11_PARSE(app, argc, argv);

	// Do nothing if no msh files are given
	if (files.size() == 0 && animations.size() == 0 && models.size() == 0)
	{
		MSH2FBX::Log(app.help());
		return 0;
	}

	// crawl for all msh files if directories are given
	files = MSH2FBX::GetFiles(files, ".msh", recOpt->count() > 0);
	animations = MSH2FBX::GetFiles(animations, ".msh", recOpt->count() > 0);
	models = MSH2FBX::GetFiles(models, ".msh", recOpt->count() > 0);

	// allow everything by default
	Converter::ModelIgnoreFilter = (EModelPurpose)0;
	for (auto it = filter.begin(); it != filter.end(); ++it)
	{
		auto& filterIT = filterMap.find(*it);
		if (filterIT != filterMap.end())
		{
			// ugly... |= operator does not work here
			Converter::ModelIgnoreFilter = (EModelPurpose)(Converter::ModelIgnoreFilter | filterIT->second);
		}
		else
		{
			MSH2FBX::Log("'"+*it+"' is not a valid filter option!");
		}
	}

	// if no FBX file name is specified, just take the first msh file name there is
	if (fbxFileName == "")
	{
		if (files.size() > 0)
		{
			fbxFileName = MSH2FBX::RemoveFileExtension(files[0]) + ".fbx";
		}
		else if (models.size() > 0)
		{
			fbxFileName = MSH2FBX::RemoveFileExtension(models[0]) + ".fbx";
		}
		else if (animations.size() > 0)
		{
			fbxFileName = MSH2FBX::RemoveFileExtension(animations[0]) + ".fbx";
		}
	}

	Converter::Start(fbxFileName);

	// Import Models first, ignoring Animations
	Converter::ChunkFilter = EChunkFilter::Animations;
	for (auto& it = models.begin(); it != models.end(); ++it)
	{
		MSH* msh = MSH::Create();
		msh->ReadFromFile(*it);
		Converter::AddMSH(msh);
		MSH::Destroy(msh);
	}

	// Import complete Files second. These can include both, Models and Animations
	Converter::ChunkFilter = EChunkFilter::None;
	for (auto& it = files.begin(); it != files.end(); ++it)
	{
		MSH* msh = MSH::Create();
		msh->ReadFromFile(*it);
		Converter::AddMSH(msh);
		MSH::Destroy(msh);
	}

	// Import Animations at last, so all Bones will be there
	Converter::ChunkFilter = EChunkFilter::Models;
	for (auto& it = animations.begin(); it != animations.end(); ++it)
	{
		MSH* msh = MSH::Create();
		msh->ReadFromFile(*it);

		if (overOpt->count() > 0)
		{
			Converter::OverrideAnimName = MSH2FBX::GetFileName(MSH2FBX::RemoveFileExtension(*it));
		}

		Converter::AddMSH(msh);
		MSH::Destroy(msh);
	}
	Converter::OverrideAnimName = "";

	Converter::Save();
	return 0;
}