#pragma once
#include "time.hpp"

namespace Engine
{
	namespace Debug
	{
#if defined(_DEBUG)
		void LogTimeStats(const Time& time);
		void LogSystem();
#else
		inline void LogTimeStats(Time& time) {};

#endif
}
	}