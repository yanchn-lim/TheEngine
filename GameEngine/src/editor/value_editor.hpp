#pragma once

#include <string>

namespace Ludus::Serialization
{
	class LSceneValue;
}

namespace Ludus::Editor
{
	struct ValueEditResult
	{
		bool changed = false;
		bool finished = false;
	};

	ValueEditResult DrawValue(
		const std::string& label,
		Ludus::Serialization::LSceneValue& value);

	ValueEditResult DrawObjectFields(Ludus::Serialization::LSceneValue& value);
}
