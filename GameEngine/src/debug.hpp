#pragma once


namespace Debug
{
	template<typename... Args>
	void Log(Args... a)
	{
		(std::cout << ... << a) << "\n";
	}
}