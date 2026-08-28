#pragma once

#include <string>

namespace Ludus::Serialization
{
	class LSceneValue;
}

namespace Ludus::Editor
{
	// changed reports a value change in this frame.
	// finished reports that an edited scalar widget became inactive.
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
