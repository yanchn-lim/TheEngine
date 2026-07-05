#include "mesh_source_data.hpp"


namespace Assets
{
	Graphics::VertexLayout CreateMeshVertexLayout()
	{
		Graphics::VertexLayout layout;
		layout.Add(0, Graphics::ShaderDataType::FLOAT3); //position
		layout.Add(1, Graphics::ShaderDataType::FLOAT3); //color
		layout.Add(2, Graphics::ShaderDataType::FLOAT2); //texcoord0

		return layout;
	}
}
