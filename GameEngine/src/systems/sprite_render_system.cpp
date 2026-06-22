#include "sprite_render_system.hpp"

#include "scene/scene.hpp"
#include "graphics/drawcmd.hpp"
#include "graphics/renderer.hpp"

void SpriteRenderSystem::SubmitDrawCommands(
	const Scene& scene,
	Graphics::Renderer& renderer,
	const Graphics::SpriteRenderResources& resources
) const
{
	for (const GameObject& go : scene.GetGameObjects())
	{
		if (!go.enabled || !go.sprite.visible) continue;

		Graphics::DrawCmd cmd;
		cmd.mesh = resources.quadMesh;
		cmd.shader = resources.shader;
		cmd.texture = resources.fallbackTexture;
		cmd.transform = go.transform.GetMatrix();

		renderer.Submit(cmd);
	}
}
