#pragma once

#include <string>

#include "lscene_value.hpp"

namespace Ludus::Serialization
{
	// the version 1 writer emits canonical CRLF text and rejects values that
	// the format cannot represent.
	class LSceneWriter
	{
	public:
		static bool Write(
			const LSceneValue& root,
			std::string& output,
			std::string& error);
	};
}
