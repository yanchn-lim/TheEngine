#pragma once

#include <string>

#include "lscene_value.hpp"

namespace Ludus::Serialization
{
	class LSceneWriter
	{
	public:
		static bool Write(
			const LSceneValue& root,
			std::string& output,
			std::string& error);
	};
}
