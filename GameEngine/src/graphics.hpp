#pragma once

//collect draw data
//figure out using the camera which is visible
//cull unseen renderers
//build rendergraph
//do render passes
//do draw calls

namespace Graphics
{
	
}

struct DrawCommand
{
	std::string shaderName;
	std::string meshName;
	float3 position;
	float3 size;
	float rotation;
};

class Renderer
{
public:
	static Renderer& Get() { static Renderer instance; return instance; }
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	bool Init();
	void Shutdown();

	void Begin();
	void End();
	void Queue(DrawCommand cmd);

	// call once per frame before End(), after Begin()
	void SetCamera(const mat4& viewProjection);

private:
	Renderer() = default;
	std::vector<DrawCommand> _commandBuffer{};
	mat4 _viewProjection = mat4(1.f);

	void Sort();
	void Flush();
};