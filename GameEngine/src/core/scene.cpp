#include "scene.hpp"
#include "graphics/renderer.hpp"
#include "graphics/mesh.hpp"
#include "graphics/primitive2d.hpp"
#include "graphics/drawcmd.hpp"

GameObject& Scene::CreateObject()
{

}

void Scene::AddObject(GameObject& obj)
{

}

void Scene::Update()
{

}

void Scene::SubmitDrawCommands(Graphics::Renderer& renderer)
{

	for (GameObject& go : _gameObjects)
	{
		//temp
		Graphics::Mesh mesh;
		auto quad = Graphics::Primitive2D::Quad();
		mesh.Create(quad.vertices, quad.vertexCount, quad.floatsPerVertex);

		//Graphics::DrawCmd cmd{ &mesh,&go., };
	}
}

void Scene::Draw()
{

}