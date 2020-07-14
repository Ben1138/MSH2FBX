// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#endif //_WIN32



// reference additional headers your program requires here
#include <string>
#include <vector>
#include <queue>
#include <functional>
#include <map>
#include <filesystem>

namespace MSH2FBX
{
	using std::string;
	using std::vector;
	using std::queue;
	using std::unique_ptr;
	using std::function;
	using std::map;
}

#include "../ConverterLib/ConverterLib.h"