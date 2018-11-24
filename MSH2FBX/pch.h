#ifndef PCH_H
#define PCH_H

// TODO: Fügen Sie hier Header hinzu, die vorkompiliert werden sollen.
#include "Converter.h"

#endif //PCH_H

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <type_traits>
#include <cmath>
#include <algorithm>
#include <functional>
#include <map>
#include <filesystem>

namespace MSH2FBX
{
	using std::clamp;
	using std::string;
	using std::vector;
	using std::queue;
	using std::unique_ptr;
	using std::function;
	using std::map;
}

#include "LibSWBF2.h"