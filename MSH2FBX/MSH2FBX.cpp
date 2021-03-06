#include "pch.h"
#include "MSH2FBX.h"

namespace MSH2FBX
{
	void Log(const char* msg)
	{
		if (IsInProgress)
		{
			char space[80];
			memset(space, ' ', 79);
			space[79] = 0;	// null termination
			std::cout << '\r' << space << '\r' << msg << std::endl;
		}
		else
		{
			std::cout << msg << std::endl;
		}
	}

	void Log(const string& msg)
	{
		Log(msg.c_str());
	}

	void ReceiveLogFromConverter(const char* msg, const uint8_t type)
	{
		Log("[LibSWBF2] " + string(msg));
	}

	void ShowProgress(const string& text, const float progress)
	{
		IsInProgress = true;

		const char ProgressBarWidth = 32;
		const float percent = ceil(progress * 100);
		const float bar = progress * ProgressBarWidth;

		std::cout << '\r' << percent << '%' << (percent / 100 >= 1 ? "" : percent / 10 >= 1 ? " " : "  ") << " [";

		for (char i = 0; i < ProgressBarWidth; ++i)
		{
			std::cout << (i < bar ? '=' : ' ');
		}

		std::cout << "] ";

		// at this point, we are at pos 40 (40 left)
		char processText[41];
		memset(processText, ' ', 40);
		processText[40] = 0; // null termination
		if (text.size() <= 40)
		{
			memcpy(processText, text.c_str(), text.size());
		}
		else
		{
			memcpy(processText, text.c_str(), 37);
			memset(&processText[37], '.', 3);
		}

		std::cout << processText;
		std::cout.flush();
	}

	void FinishProgress(string FinMsg)
	{
		ShowProgress(FinMsg, 1.0f);
		IsInProgress = false;
		std::cout << std::endl;
	}

	bool IsDirectory(const fs::path Path)
	{
		fs::path filename = Path.filename();
		return filename == "" || filename == "." || filename == "..";
	}
	
	vector<fs::path> GetFiles(const fs::path& Directory, const string& Extension, const bool recursive)
	{
		vector<fs::path> mshFiles;
		if (!fs::exists(Directory))
		{
			return mshFiles;
		}

		for (auto p : fs::directory_iterator(Directory))
		{
			if (p.path().extension() == Extension)
			{
				mshFiles.insert(mshFiles.end(), p.path().string());
			}

			if (recursive && p.is_directory())
			{
				const vector<fs::path>& next = GetFiles(p.path(), Extension, recursive);
				mshFiles.insert(mshFiles.end(), next.begin(), next.end());
			}
		}
		return mshFiles;
	}

	vector<fs::path> GetFiles(const vector<fs::path>& Paths, const string& Extension, const bool recursive)
	{
		vector<fs::path> files;
		for (auto it = Paths.begin(); it != Paths.end(); ++it)
		{
			if (fs::exists(*it))
			{
				if (fs::is_directory(*it))
				{
					const vector<fs::path>& next = GetFiles(*it, Extension, recursive);
					files.insert(files.end(), next.begin(), next.end());
				}
				else if (fs::is_regular_file(*it))
				{
					files.insert(files.end(), *it);
				}
			}
			else
			{
				Log((*it).u8string() + " does not exist!");
			}
		}
		return files;
	}

	bool ProcessMSH(fs::path mshPath, const bool overrideAnimName, Converter& converter, const bool createFBXFile)
	{
		if (createFBXFile)
		{
			fs::path fbxPath = mshPath;
			fbxPath.replace_extension(".fbx");
			
			converter.Close();
			if (!converter.Start(fbxPath))
			{
				Log("converter.Start failed!");
				return false;
			}
		}

		if (overrideAnimName)
		{
			converter.OverrideAnimName = mshPath.filename().replace_extension("").u8string();
		}

		if (!converter.AddMSH(mshPath))
		{
			if (createFBXFile)
			{
				converter.Close();
			}
			return false;
		}

		converter.OverrideAnimName = "";

		if (createFBXFile)
		{
			return converter.SaveFBX();
		}
		return true;
	}
}

// the main function must not lie inside a namespace
int main(int argc, char *argv[])
{
#if _DEBUG
	std::cin.get();
#endif

	using std::string;
	using std::vector;
	using std::map;
	using namespace MSH2FBX;

	//Log(string("float: ") + std::to_string(sizeof(float_t)));
	//Log(string("BaseChunk: ") + std::to_string(sizeof(LibSWBF2::Chunks::BaseChunk)));
	//Log(string("MSH: ") + std::to_string(sizeof(LibSWBF2::Chunks::Mesh::MSH)));
	//Log(string("MSH2: ") + std::to_string(sizeof(LibSWBF2::Chunks::Mesh::MSH2)));
	//Log(string("SINF: ") + std::to_string(sizeof(LibSWBF2::Chunks::Mesh::SINF)));
	//Log(string("STR: ") + std::to_string(sizeof(LibSWBF2::Chunks::Mesh::STR)));
	//Log(string("FRAM: ") + std::to_string(sizeof(LibSWBF2::Chunks::Mesh::FRAM)));
	//Log(string("BBOX: ") + std::to_string(sizeof(LibSWBF2::Chunks::Mesh::BBOX)));
	//Log(string("String: ") + std::to_string(sizeof(LibSWBF2::Types::String)));
	//Log(string("Vector2: ") + std::to_string(sizeof(LibSWBF2::Types::Vector2)));
	//Log(string("Vector3: ") + std::to_string(sizeof(LibSWBF2::Types::Vector3)));
	//Log(string("Vector4: ") + std::to_string(sizeof(LibSWBF2::Types::Vector4)));
	//Log(string("Color: ") + std::to_string(sizeof(LibSWBF2::Types::Color)));
	//return 0;

	vector<fs::path> files;
	vector<fs::path> animations;
	vector<fs::path> models;
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

		// Unknown purpose
		{"Miscellaneous", EModelPurpose::Miscellaneous},
	};
	vector<string> filter;
	fs::path fbxDestination = "";
	fs::path mshBaseposeFile = "";

	// Build up Command Line Parser
	CLI::App app
	{
		"--------------------------------------------------------------\n"
		"-------------------- MSH to FBX Converter --------------------\n"
		"--------------------------------------------------------------\n"
		"Web: https://github.com/Ben1138/MSH2FBX \n"
		"\n"
		"This tool can convert multiple MSHs at once! If no destination\n"
		"is specified (via the -d option), all resulting FBX files will\n"
		"be placed in the exact same location where the respective MSH\n"
		"files lie.\n"
		"If some directory is specified, the resulting FBX files will end\n"
		"up in that specific directory (directory must exist!)\n"
		"If a specific FBX File Name is given, all MSHs will be merged\n"
		"into that single FBX File!\n"
		"\n"
		"Note that all resulting FBX Files will be overwritten\n"
		"without question!\n"
	};

	CLI::Option* filesOpt = app.add_option("-f,--files", files, "MSH file paths (file or directory, one or more), importing everything.")->check(CLI::ExistingPath);
	CLI::Option* animsOpt = app.add_option("-a,--animations", animations, "MSH file Paths (file or directory, one or more), importing Animation Data only!")->check(CLI::ExistingPath);
	CLI::Option* modelsOpt = app.add_option("-m,--models", models, "MSH file paths (file or directory, one or more), importing Model Data only!")->check(CLI::ExistingPath);
	CLI::Option* destOpt = app.add_option("-d,--destination", fbxDestination, "Folder destination or specific FBX File Name (see details above!)");
	CLI::Option* baseOpt = app.add_option("-b,--basepose", mshBaseposeFile, "Specific MSH Basepose to use for Bone import.")->check(CLI::ExistingFile);
	CLI::Option* overOpt = app.add_flag("-o,--override-anim-name", "Will use the MSH file name as Animation name, rather than the Animation name stored inside the MSH file.");
	CLI::Option* recOpt = app.add_flag("-r,--recursive", "For all given directories, crawling will be recursive (will include all sub-directories).");
	CLI::Option* emptOpt = app.add_flag("-e,--empty-meshes", "Meshes won't be processed and will end up empty. This is usefull to convert Animations.");
	CLI::Option* printOpt = app.add_flag("-p,--print-hierarchy", "Print the hierarchy of the resulting FBX file(s).");

	string filterOptionInfo = "What to ignore. Options are:\n";
	for (auto it = filterMap.begin(); it != filterMap.end(); ++it)
	{
		filterOptionInfo += "\t\t\t\t" + it->first + "\n";
	}
	app.add_option("-i,--ignore", filter, filterOptionInfo);

	// *parse magic*
	CLI11_PARSE(app, argc, argv);

	bool singleFbxFile = false;
	fs::path filename = fbxDestination.filename();
	if (!fbxDestination.empty())
	{
		if (IsDirectory(fbxDestination) && !fs::exists(fbxDestination))
		{
			Log("Given destination directory does not exist!");
			Log(app.help());
			return 0;
		}
		else if (fbxDestination.extension() != ".fbx")
		{
			Log("WARNING: Your desired FBX File Name does not have the required .fbx extension!");
			Log(app.help());
			return 0;
		}
		else
		{
			singleFbxFile = true;
		}
	}

	if (!singleFbxFile && mshBaseposeFile != "")
	{
		Log("Cannot apply Basepose in multi export mode! You need to specify a single FBX to merge everything to!");
		Log(app.help());
		return 0;
	}

	// Do nothing if no msh files are given
	if (files.size() == 0 && animations.size() == 0 && models.size() == 0)
	{
		Log("No MSH files given!");
		Log(app.help());
		return 0;
	}

	// crawl for all msh files if directories are given
	files = GetFiles(files, ".msh", recOpt->count() > 0);
	animations = GetFiles(animations, ".msh", recOpt->count() > 0);
	models = GetFiles(models, ".msh", recOpt->count() > 0);

	const bool overrideAnimName = overOpt->count() > 0;
	const size_t numFiles = files.size() + models.size() + animations.size();
	size_t fileCounter = 0;
	size_t successCounter = 0;

	Converter converter;
	converter.SetLogCallback(&ReceiveLogFromConverter);
	converter.bEmptyMeshes = emptOpt->count() > 0;
	converter.bPrintHierachy = printOpt->count() > 0;
	converter.BaseposeMSH = mshBaseposeFile;
	converter.Start(fbxDestination);

	// allow everything by default
	converter.ModelIgnoreFilter = (EModelPurpose)0;
	for (auto it = filter.begin(); it != filter.end(); ++it)
	{
		auto filterIT = filterMap.find(*it);
		if (filterIT != filterMap.end())
		{
			// ugly... |= operator does not work here
			converter.ModelIgnoreFilter = (EModelPurpose)(converter.ModelIgnoreFilter | filterIT->second);
		}
		else
		{
			Log("'" + *it + "' is not a valid filter option!");
		}
	}

	// Import Models first (specified with -m), ignoring Animations
	converter.ChunkFilter = EChunkFilter::Animations;
	for (auto it = models.begin(); it != models.end(); ++it)
	{
		ShowProgress((*it).filename().u8string(), (float)(fileCounter++) / numFiles);
		if (ProcessMSH(*it, overrideAnimName, converter, !singleFbxFile))
		{
			++successCounter;
		}
	}

	// Import complete Files second (specified with -f). These can include both, Models and Animations
	converter.ChunkFilter = EChunkFilter::None;
	for (auto it = files.begin(); it != files.end(); ++it)
	{
		ShowProgress((*it).filename().u8string(), (float)(fileCounter++) / numFiles);
		if (ProcessMSH(*it, overrideAnimName, converter, !singleFbxFile))
		{
			++successCounter;
		}
	}

	// Import Animations at last (specified with -a), so all Bones will be there
	converter.ChunkFilter = EChunkFilter::Models;
	for (auto it = animations.begin(); it != animations.end(); ++it)
	{
		ShowProgress((*it).filename().u8string(), (float)(fileCounter++) / numFiles);
		if (ProcessMSH(*it, overrideAnimName, converter, !singleFbxFile))
		{
			++successCounter;
		}
	}

	if (singleFbxFile)
	{
		if (successCounter == 0)
		{
			converter.Close();
		}
		else
		{
			ShowProgress("Saving...", 0.99f);
			converter.SaveFBX();
			converter.Close();
		}
	}

	FinishProgress(successCounter > 0 ? "Done!" : "No files processed...");

#if _DEBUG
	std::cin.get();
#endif
	return 0;
}