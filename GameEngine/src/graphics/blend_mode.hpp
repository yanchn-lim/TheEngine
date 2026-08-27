#pragma once

namespace Ludus::Graphics
{
	// describes blend intent so each back end can select native factors
	enum class BlendMode
	{
		NONE,
		ALPHA,
		ADDITIVE,
		PREMULTIPLIED_ALPHA,
		MULTIPLY
	};
}
