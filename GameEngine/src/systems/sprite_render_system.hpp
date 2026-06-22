#pragma once

class Scene;

namespace Graphics
{
	class Renderer;
	struct SpriteRenderResources;
}

class SpriteRenderSystem
{
public:
	void SubmitDrawCommands(
		const Scene& scene,
		Graphics::Renderer& renderer,
		const Graphics::SpriteRenderResources& resources
	) const;
};
