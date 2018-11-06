#include "pch.h"
#include "MSH2FBX.h"
#include "Converter.h"

namespace MSH2FBX
{
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
}

// the main function must not lie inside a namespace
int main()
{
	using std::string;
	using std::vector;
	using LibSWBF2::Logging::Logger;
	using LibSWBF2::Chunks::Mesh::MSH;
	using MSH2FBX::Converter;
	using MSH2FBX::EChunkFilter;
	using MSH2FBX::EModelPurpose;

	Logger::SetLogCallback(MSH2FBX::LogCallback);

	const vector<string> Models
	{
		"Testing\\rep_inf_ep3trooper.msh"
	};

	const vector<string> Animations
	{
		"Testing\\human_rifle_stand_walkforward.msh",
		"Testing\\human_rifle_stand_runforward.msh",
		"Testing\\human_rifle_standalert_runforward.msh",
		"Testing\\human_rifle_standalert_walkforward.msh",
		"Testing\\human_rifle_stand_reload_full.msh"
	};

	Converter::ModelIgnoreFilter = (EModelPurpose)(
		EModelPurpose::Mesh_ShadowVolume |
		EModelPurpose::Mesh_Lowrez |
		EModelPurpose::Mesh_Collision |
		EModelPurpose::Mesh_VehicleCollision |
		EModelPurpose::Mesh_TerrainCut
	);
	Converter::Start(MSH2FBX::RemoveFileExtension(Models[0]) + ".fbx");

	Converter::ChunkFilter = EChunkFilter::Animations;
	for (auto it = Models.begin(); it != Models.end(); ++it)
	{
		MSH* msh = MSH::Create();
		msh->ReadFromFile(*it);
		Converter::AddMSH(msh);
		MSH::Destroy(msh);
	}

	Converter::ChunkFilter = EChunkFilter::Models;
	for (auto it = Animations.begin(); it != Animations.end(); ++it)
	{
		MSH* msh = MSH::Create();
		msh->ReadFromFile(*it);
		Converter::OverrideAnimName = MSH2FBX::GetFileName(MSH2FBX::RemoveFileExtension(*it));
		Converter::AddMSH(msh);
		MSH::Destroy(msh);
	}
	Converter::OverrideAnimName = "";

	Converter::Save();
	return 0;
}