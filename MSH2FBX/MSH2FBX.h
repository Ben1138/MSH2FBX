#pragma once
#include "pch.h"

namespace MSH2FBX
{
	using namespace LibSWBF2::Logging;
	using namespace LibSWBF2::Chunks::Mesh;
	using namespace LibSWBF2::Types;

	static bool IsInProgress = false;

	void Log(const char* msg);
	void Log(const string& msg);
	void LogEntry(LoggerEntry entry);
	void ShowProgress(const float progress);
	void FinishProgress();

	string GetFileName(const string& path);
	string RemoveFileExtension(const string& fileName);
	vector<string> GetFiles(string Directory, string Extension, bool recursive);
	vector<string> GetFiles(vector<string> Paths, string Extension, bool recursive);
}