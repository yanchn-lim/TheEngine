#pragma once

#include <string>

#include "serialization/lscene_value.hpp"

namespace Ludus
{
	struct SceneLoadError
	{
		std::string message;
		Serialization::SourceLocation location;
		std::string path;
	};
}
