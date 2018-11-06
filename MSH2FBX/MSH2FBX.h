#pragma once
#include "pch.h"

namespace MSH2FBX
{
	using namespace LibSWBF2::Logging;
	using namespace LibSWBF2::Chunks::Mesh;
	using namespace LibSWBF2::Types;

	void Log(const char* msg);
	void Log(const string& msg);
	void LogEntry(LoggerEntry entry);

	string GetFileName(const string& path);
	string RemoveFileExtension(const string& fileName);
}