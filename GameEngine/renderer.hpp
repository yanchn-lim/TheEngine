#pragma once



namespace Graphics
{
	struct RenderCmd;

	class Renderer
	{
	private:

	public:
		void Init();
		void Submit(RenderCmd);
		void BeginFrame();
		void EndFrame();
		void Shutdown();
	};
}