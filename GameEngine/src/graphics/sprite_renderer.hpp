#pragma once

#include "mesh.hpp"
#include "shader.hpp"
#include "texture2d.hpp"


namespace Graphics
{
	class SpriteRenderer
	{
	private:
		Mesh _quadMesh;
		Shader _shader;
	
	public:
		bool Init();
		void Shutdown();

		void DrawSprite();
	};
}
