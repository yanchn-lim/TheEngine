#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "lscene_value.hpp"

namespace Ludus::Serialization
{
	struct LSceneParseError
	{
		std::string message;
		SourceLocation location;
	};

	struct LSceneParseResult
	{
		LSceneValue root = LSceneValue::ObjectValue();
		std::string resourceType;
		std::string resourceName;
		std::vector<LSceneParseError> errors;

		explicit operator bool() const noexcept
		{
			return errors.empty();
		}
	};

	class LSceneParser
	{
	public:
		LSceneParseResult Parse(std::string_view source) const;
	};
}
