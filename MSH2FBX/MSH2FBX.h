#pragma once
#include "pch.h"
#include <filesystem>

namespace MSH2FBX
{
	using namespace LibSWBF2::Logging;
	using namespace LibSWBF2::Chunks::Mesh;
	using namespace LibSWBF2::Types;
	namespace fs = std::filesystem;

	static bool IsInProgress = false;

	void Log(const char* msg);
	void Log(const string& msg);
	void LogEntry(LoggerEntry entry);
	void ShowProgress(const float progress);
	void FinishProgress();

	bool DescribesDirectory(const fs::path Path);
	vector<fs::path> GetFiles(const fs::path& Directory, const string& Extension, const bool recursive);
	vector<fs::path> GetFiles(const vector<fs::path>& Paths, const string& Extension, const bool recursive);

	bool ProcessMSH(fs::path filename, const bool overrideAnimName, const bool createFBXFile);
}