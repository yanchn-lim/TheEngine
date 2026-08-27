#include "mesh_import_data.hpp"


namespace Ludus::Assets
{
	Ludus::Graphics::VertexLayout CreateMeshVertexLayout()
	{
		Ludus::Graphics::VertexLayout layout;
		layout.Add(0, Ludus::Graphics::ShaderDataType::FLOAT3); //position
		layout.Add(1, Ludus::Graphics::ShaderDataType::FLOAT3); //color
		layout.Add(2, Ludus::Graphics::ShaderDataType::FLOAT2); //texcoord0

		return layout;
	}
}
