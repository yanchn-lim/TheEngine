#include "file_system.hpp"

#include <string>
#include <sstream>

namespace Ludus::FileSystem
{
	bool ReadTextFile(const char* path, std::string& out)
	{
		// Read the whole text file into memory for shader/source asset loading.
		std::ifstream file(path, std::ios::in);

		if (!file.is_open()) return false;

		std::ostringstream oss;
		oss << file.rdbuf();

		out = oss.str();

		return true;
	}
}

