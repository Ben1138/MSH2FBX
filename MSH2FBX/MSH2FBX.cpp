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
}

// the main function must not lie inside a namespace
int main()
{
	const char* FILENAME = "rep_inf_ep3trooper.msh";

	using LibSWBF2::Logging::Logger;
	using LibSWBF2::Chunks::Mesh::MSH;
	using MSH2FBX::Converter;

	Logger::SetLogCallback(MSH2FBX::LogCallback);

	MSH* msh = MSH::Create();
	msh->ReadFromFile(FILENAME);
	Converter::SaveAsFBX(msh, Converter::GetPlainName(FILENAME) + ".fbx");
	MSH::Destroy(msh);

	return 0;
}