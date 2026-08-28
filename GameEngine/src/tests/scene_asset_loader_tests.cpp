#include "scene_asset_loader_tests.hpp"

#include <string_view>

#include "assets/asset_manager.hpp"
#include "scene/scene_asset_loader.hpp"
#include "serialization/lscene_parser.hpp"

namespace Tests
{
	bool RunSceneAssetLoaderTests()
	{
		constexpr std::string_view validSource =
			"scene \"Asset Test\"\n"
			"version: 1\n"
			"assets\n"
			"\tmaterials\n"
			"\t\tmaxwell\n"
			"\t\t\tsource: \"assets/materials/maxwell.lmaterial\"\n"
			"\tmeshes\n"
			"\t\tmaxwell\n"
			"\t\t\tsource: \"assets/models/maxwell.obj\"\n";

		const Ludus::Serialization::LSceneParseResult parsed =
			Ludus::Serialization::LSceneParser{}.Parse(validSource);
		if (!parsed)
			return false;

		Ludus::Assets::AssetManager assets;
		const Ludus::Assets::ShaderHandle shaderResource =
			assets.LoadShaderResource("assets/shaders/standard.lshader");
		const Ludus::Assets::ShaderHandle normalizedShaderResource =
			assets.LoadShaderResource("assets/shaders/./standard.lshader");
		if (!shaderResource || shaderResource.id != normalizedShaderResource.id ||
			assets.LoadShaderResource("assets/shaders/missing.lshader"))
		{
			return false;
		}
		const Ludus::Assets::MaterialHandle materialResource =
			assets.LoadMaterialResource("assets/materials/maxwell.lmaterial");
		const Ludus::Assets::MaterialHandle normalizedMaterialResource =
			assets.LoadMaterialResource("assets/materials/./maxwell.lmaterial");
		const Ludus::Assets::MaterialAsset* materialAsset = assets.Get(materialResource);
		if (!materialResource || materialResource.id != normalizedMaterialResource.id ||
			!materialAsset || materialAsset->shader.id != shaderResource.id ||
			!materialAsset->texture || !materialAsset->state.depthTest ||
			!materialAsset->state.depthWrite || !materialAsset->state.culling ||
			materialAsset->state.blendMode != Ludus::Graphics::BlendMode::NONE ||
			assets.LoadMaterialResource("assets/materials/missing.lmaterial"))
		{
			return false;
		}

		Ludus::SceneAssetContext context;
		std::vector<Ludus::SceneLoadError> errors;
		if (!Ludus::SceneAssetLoader::Load(
			parsed.root, "tests/asset_test.lscene", assets, context, errors) ||
			!errors.empty() || context.materials.size() != 1 ||
			context.meshes.size() != 1)
		{
			return false;
		}

		constexpr std::string_view invalidSource =
			"scene \"Invalid Asset Test\"\n"
			"version: 1\n"
			"assets\n"
			"\tmaterials\n"
			"\t\tbroken\n"
			"\t\t\tsource: \"assets/materials/missing.lmaterial\"\n";

		const Ludus::Serialization::LSceneParseResult invalidParsed =
			Ludus::Serialization::LSceneParser{}.Parse(invalidSource);
		if (!invalidParsed)
			return false;

		Ludus::SceneAssetContext invalidContext;
		std::vector<Ludus::SceneLoadError> invalidErrors;
		if (Ludus::SceneAssetLoader::Load(
			invalidParsed.root,
			"tests/invalid_asset_test.lscene",
			assets,
			invalidContext,
			invalidErrors) || invalidErrors.empty() ||
			!invalidContext.materials.empty() || !invalidContext.meshes.empty())
		{
			return false;
		}

		constexpr std::string_view invalidDefinitionSource =
			"scene \"Invalid Definition\"\n"
			"version: 1\n"
			"assets\n"
			"\tmaterials\n"
			"\t\tstandard\n"
			"\t\t\tsource: \"assets/materials/maxwell.lmaterial\"\n"
			"\t\t\tunknown: true\n";
		const Ludus::Serialization::LSceneParseResult invalidDefinition =
			Ludus::Serialization::LSceneParser{}.Parse(invalidDefinitionSource);
		if (!invalidDefinition)
			return false;

		Ludus::SceneAssetContext invalidDefinitionContext;
		std::vector<Ludus::SceneLoadError> invalidDefinitionErrors;
		if (Ludus::SceneAssetLoader::Load(
			invalidDefinition.root,
			"tests/invalid_definition.lscene",
			assets,
			invalidDefinitionContext,
			invalidDefinitionErrors) ||
			invalidDefinitionErrors.empty() ||
			!invalidDefinitionContext.materials.empty())
		{
			return false;
		}

		constexpr std::string_view firstCollisionSource =
			"scene \"First Collision Scene\"\n"
			"version: 1\n"
			"assets\n"
			"\tmaterials\n"
			"\t\tshared\n"
			"\t\t\tsource: \"assets/materials/maxwell.lmaterial\"\n"
			"\tmeshes\n"
			"\t\tshared\n"
			"\t\t\tsource: \"assets/models/test_triangle.obj\"\n";

		constexpr std::string_view secondCollisionSource =
			"scene \"Second Collision Scene\"\n"
			"version: 1\n"
			"assets\n"
			"\tmaterials\n"
			"\t\tshared\n"
			"\t\t\tsource: \"assets/materials/maxwell.lmaterial\"\n"
			"\tmeshes\n"
			"\t\tshared\n"
			"\t\t\tsource: \"assets/models/test_quad.obj\"\n";

		const auto firstCollision = Ludus::Serialization::LSceneParser{}.Parse(firstCollisionSource);
		const auto secondCollision = Ludus::Serialization::LSceneParser{}.Parse(secondCollisionSource);
		if (!firstCollision || !secondCollision)
			return false;

		Ludus::SceneAssetContext firstContext;
		Ludus::SceneAssetContext secondContext;
		std::vector<Ludus::SceneLoadError> collisionErrors;
		if (!Ludus::SceneAssetLoader::Load(
				firstCollision.root, "tests/first.lscene", assets, firstContext, collisionErrors) ||
			!Ludus::SceneAssetLoader::Load(
				secondCollision.root, "tests/second.lscene", assets, secondContext, collisionErrors))
		{
			return false;
		}

		const Ludus::Assets::MeshHandle firstMesh = firstContext.meshes.at("shared");
		const Ludus::Assets::MeshHandle secondMesh = secondContext.meshes.at("shared");
		const Ludus::Assets::MaterialHandle firstMaterial = firstContext.materials.at("shared");
		const Ludus::Assets::MaterialHandle secondMaterial = secondContext.materials.at("shared");
		const Ludus::Assets::MeshAsset* firstMeshAsset = assets.Get(firstMesh);
		const Ludus::Assets::MeshAsset* secondMeshAsset = assets.Get(secondMesh);
		if (firstMesh.id == secondMesh.id || firstMaterial.id != secondMaterial.id ||
			!firstMeshAsset || !secondMeshAsset ||
			firstMeshAsset->label != "tests/first.lscene::mesh::shared" ||
			secondMeshAsset->label != "tests/second.lscene::mesh::shared" ||
			firstMeshAsset->surfaces.front().vertices.size() ==
				secondMeshAsset->surfaces.front().vertices.size())
		{
			return false;
		}

		std::vector<Ludus::SceneLoadError> previousErrors{
			{ "previous error", { 1, 1 } }
		};
		Ludus::SceneAssetContext appendSafeContext;
		return Ludus::SceneAssetLoader::Load(
			parsed.root,
			"tests/asset_test.lscene",
			assets,
			appendSafeContext,
			previousErrors) && previousErrors.size() == 1;
	}
}
