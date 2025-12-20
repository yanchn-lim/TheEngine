#pragma once

namespace Engine
{
	class Engine
	{
	public:
		//bang
		int Run();
	protected:
		void OnInitialize();
		void OnUpdate();
		void OnFixedUpdate();
		void MainLoop();
	};
}