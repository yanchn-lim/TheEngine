#include "scene_loader_tests.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string_view>
#include <utility>

#include <glm/gtc/quaternion.hpp>

#include "assets/asset_manager.hpp"
#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "position.hpp"
#include "rotator.hpp"
#include "scene/scene.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/scene_loader.hpp"
#include "scene/scene_serializer.hpp"
#include "serialization/lscene_parser.hpp"
#include "serialization/lscene_writer.hpp"

namespace Tests
{
	namespace
	{
		bool LoadOrderedComponents(
			const char* path,
			Components::Transform& transform,
			Rotator& rotator)
		{
			Assets::AssetManager assets;
			Ludus::Scene scene;
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			RegisterRotatorSceneComponent(components);

			std::vector<Ludus::SceneLoadError> errors;
			if (!Ludus::SceneLoader::Load(path, scene, assets, components, errors))
				return false;

			const ECS::Entity entity = scene.FindEntity("test_entity");
			const Components::Transform* loadedTransform =
				scene.GetWorld().TryGetComponent<Components::Transform>(entity);
			const Rotator* loadedRotator = scene.GetWorld().TryGetComponent<Rotator>(entity);
			if (!loadedTransform || !loadedRotator)
				return false;

			transform = *loadedTransform;
			rotator = *loadedRotator;
			return true;
		}

		bool ComponentOrderIsIndependent()
		{
			Components::Transform firstTransform;
			Components::Transform secondTransform;
			Rotator firstRotator;
			Rotator secondRotator;
			if (!LoadOrderedComponents(
					"assets/scenes/tests/component_order_a.lscene",
					firstTransform,
					firstRotator) ||
				!LoadOrderedComponents(
					"assets/scenes/tests/component_order_b.lscene",
					secondTransform,
					secondRotator))
			{
				return false;
			}

			return
				firstTransform.position.x == secondTransform.position.x &&
				firstTransform.position.y == secondTransform.position.y &&
				firstTransform.position.z == secondTransform.position.z &&
				firstTransform.scale.x == secondTransform.scale.x &&
				firstTransform.scale.y == secondTransform.scale.y &&
				firstTransform.scale.z == secondTransform.scale.z &&
				firstTransform.rotation.w == secondTransform.rotation.w &&
				firstTransform.rotation.x == secondTransform.rotation.x &&
				firstTransform.rotation.y == secondTransform.rotation.y &&
				firstTransform.rotation.z == secondTransform.rotation.z &&
				firstRotator.axis.x == secondRotator.axis.x &&
				firstRotator.axis.y == secondRotator.axis.y &&
				firstRotator.axis.z == secondRotator.axis.z &&
				firstRotator.radiansPerSecond == secondRotator.radiansPerSecond;
		}

		bool NearlyEqual(float left, float right)
		{
			return std::abs(left - right) <= 1.0e-4f;
		}

		Serialization::LSceneValue MakeWriterRoot()
		{
			Serialization::LSceneValue::Object root;
			root.emplace("scene", Serialization::LSceneValue::String("Test", {}));
			root.emplace("version", Serialization::LSceneValue::Integer(1, {}));
			root.emplace("entities", Serialization::LSceneValue::ObjectValue());
			return Serialization::LSceneValue::ObjectValue(std::move(root));
		}

		Serialization::LSceneValue::Object& WriterEntities(
			Serialization::LSceneValue& root)
		{
			return *root.TryGetObject()->find("entities")->second.TryGetObject();
		}

		bool WriterRejects(Serialization::LSceneValue root)
		{
			std::string output;
			std::string error;
			return !Serialization::LSceneWriter::Write(root, output, error) &&
				!error.empty();
		}

		bool WriterBoundariesAreValidated()
		{
			Serialization::LSceneValue invalidVersion = MakeWriterRoot();
			invalidVersion.TryGetObject()->insert_or_assign(
				"version", Serialization::LSceneValue::Integer(2, {}));

			Serialization::LSceneValue missingEntities = MakeWriterRoot();
			missingEntities.TryGetObject()->erase("entities");

			Serialization::LSceneValue invalidIdentifier = MakeWriterRoot();
			WriterEntities(invalidIdentifier).emplace(
				"bad id", Serialization::LSceneValue::ObjectValue());

			Serialization::LSceneValue emptyArray = MakeWriterRoot();
			Serialization::LSceneValue::Object emptyArrayEntity;
			emptyArrayEntity.emplace("value",
				Serialization::LSceneValue::ArrayValue({}, {}));
			WriterEntities(emptyArray).emplace(
				"entity", Serialization::LSceneValue::ObjectValue(std::move(emptyArrayEntity)));

			Serialization::LSceneValue nestedArray = MakeWriterRoot();
			Serialization::LSceneValue::Array inner;
			inner.push_back(Serialization::LSceneValue::Float(1.0, {}));
			Serialization::LSceneValue::Array outer;
			outer.push_back(Serialization::LSceneValue::ArrayValue(std::move(inner), {}));
			Serialization::LSceneValue::Object nestedArrayEntity;
			nestedArrayEntity.emplace("value",
				Serialization::LSceneValue::ArrayValue(std::move(outer), {}));
			WriterEntities(nestedArray).emplace(
				"entity", Serialization::LSceneValue::ObjectValue(std::move(nestedArrayEntity)));

			Serialization::LSceneValue nonFinite = MakeWriterRoot();
			Serialization::LSceneValue::Object nonFiniteEntity;
			nonFiniteEntity.emplace("value", Serialization::LSceneValue::Float(
				std::numeric_limits<double>::infinity(), {}));
			WriterEntities(nonFinite).emplace(
				"entity", Serialization::LSceneValue::ObjectValue(std::move(nonFiniteEntity)));

			Serialization::LSceneValue controlCharacter = MakeWriterRoot();
			controlCharacter.TryGetObject()->insert_or_assign(
				"scene", Serialization::LSceneValue::String("Bad\rName", {}));

			Serialization::LSceneValue escaped = MakeWriterRoot();
			const std::string escapedName = "Quote \" slash \\ line\n tab\t";
			escaped.TryGetObject()->insert_or_assign(
				"scene", Serialization::LSceneValue::String(escapedName, {}));
			std::string text;
			std::string error;
			if (!Serialization::LSceneWriter::Write(escaped, text, error))
				return false;
			const Serialization::LSceneParseResult parsed =
				Serialization::LSceneParser{}.Parse(text);
			const std::string* parsedName = parsed.root.Find("scene")
				? parsed.root.Find("scene")->TryGetString()
				: nullptr;

			return parsed && parsedName && *parsedName == escapedName &&
				WriterRejects(std::move(invalidVersion)) &&
				WriterRejects(std::move(missingEntities)) &&
				WriterRejects(std::move(invalidIdentifier)) &&
				WriterRejects(std::move(emptyArray)) &&
				WriterRejects(std::move(nestedArray)) &&
				WriterRejects(std::move(nonFinite)) &&
				WriterRejects(std::move(controlCharacter));
		}

		bool SerializationFails(
			const Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components)
		{
			std::string output = "stale";
			std::vector<std::string> errors;
			return !Ludus::SceneSerializer::Serialize(
				scene, components, output, errors) &&
				output.empty() && !errors.empty();
		}

		bool SerializerBoundariesAreValidated()
		{
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			RegisterRotatorSceneComponent(components);

			Ludus::Scene invalidId;
			invalidId.CreateEntity("bad id", "Bad ID");

			Ludus::Scene invalidName;
			invalidName.CreateEntity("entity", "Bad\rName");

			Ludus::Scene foreignMesh;
			const ECS::Entity foreignEntity = foreignMesh.CreateEntity("entity", "Entity");
			foreignMesh.GetWorld().AddComponent(
				foreignEntity,
				Components::Renderable{ Assets::MeshHandle{ 99 }, {}, true });

			Ludus::Scene ambiguousMesh;
			Ludus::SceneAssetContext ambiguousAssets;
			ambiguousAssets.meshes.emplace("first", Assets::MeshHandle{ 7 });
			ambiguousAssets.meshes.emplace("second", Assets::MeshHandle{ 7 });
			ambiguousMesh.SetSerializationData(
				"Ambiguous",
				Serialization::LSceneValue::ObjectValue(),
				std::move(ambiguousAssets));
			const ECS::Entity ambiguousEntity =
				ambiguousMesh.CreateEntity("entity", "Entity");
			ambiguousMesh.GetWorld().AddComponent(
				ambiguousEntity,
				Components::Renderable{ Assets::MeshHandle{ 7 }, {}, true });

			Ludus::Scene invalidFloat;
			const ECS::Entity invalidFloatEntity =
				invalidFloat.CreateEntity("entity", "Entity");
			Components::Transform invalidTransform;
			invalidTransform.position.x = std::numeric_limits<float>::infinity();
			invalidFloat.GetWorld().AddComponent(invalidFloatEntity, invalidTransform);

			Ludus::Scene invalidQuaternion;
			const ECS::Entity invalidQuaternionEntity =
				invalidQuaternion.CreateEntity("entity", "Entity");
			Components::Transform zeroRotation;
			zeroRotation.rotation = glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f };
			invalidQuaternion.GetWorld().AddComponent(
				invalidQuaternionEntity, zeroRotation);

			Ludus::Scene invalidRotator;
			const ECS::Entity invalidRotatorEntity =
				invalidRotator.CreateEntity("entity", "Entity");
			invalidRotator.GetWorld().AddComponent(
				invalidRotatorEntity,
				Rotator{ {}, 1.0f });

			Ludus::Scene unregisteredComponent;
			const ECS::Entity unregisteredEntity =
				unregisteredComponent.CreateEntity("entity", "Entity");
			unregisteredComponent.GetWorld().AddComponent(
				unregisteredEntity,
				Position{ 1.0f, 2.0f });

			Ludus::Scene missingMaterial;
			Ludus::SceneAssetContext missingMaterialAssets;
			missingMaterialAssets.meshes.emplace("mesh", Assets::MeshHandle{ 8 });
			missingMaterialAssets.meshHasDefaultMaterials.emplace("mesh", false);
			missingMaterial.SetSerializationData(
				"Missing Material",
				Serialization::LSceneValue::ObjectValue(),
				std::move(missingMaterialAssets));
			const ECS::Entity missingMaterialEntity =
				missingMaterial.CreateEntity("entity", "Entity");
			missingMaterial.GetWorld().AddComponent(
				missingMaterialEntity,
				Components::Renderable{ Assets::MeshHandle{ 8 }, {}, true });

			return SerializationFails(invalidId, components) &&
				SerializationFails(invalidName, components) &&
				SerializationFails(foreignMesh, components) &&
				SerializationFails(ambiguousMesh, components) &&
				SerializationFails(invalidFloat, components) &&
				SerializationFails(invalidQuaternion, components) &&
				SerializationFails(invalidRotator, components) &&
				SerializationFails(unregisteredComponent, components) &&
				SerializationFails(missingMaterial, components);
		}

		bool EmptyAndDeadScenesSerialize()
		{
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);

			Ludus::Scene source;
			const ECS::Entity removed = source.CreateEntity("removed", "Removed");
			source.GetWorld().RemoveEntity(removed);

			std::string text;
			std::vector<std::string> serializationErrors;
			if (!Ludus::SceneSerializer::Serialize(
				source, components, text, serializationErrors))
				return false;

			Assets::AssetManager assets;
			Ludus::Scene loaded;
			std::vector<Ludus::SceneLoadError> loadErrors;
			return Ludus::SceneLoader::LoadText(
				text,
				"empty_round_trip.lscene",
				loaded,
				assets,
				components,
				loadErrors) &&
				loaded.GetWorld().GetEntityCount() == 0 &&
				!loaded.FindEntity("removed").IsValid();
		}

		bool ReadBinaryFile(const std::filesystem::path& path, std::string& output)
		{
			std::ifstream file(path, std::ios::binary);
			if (!file.is_open())
				return false;
			output.assign(
				std::istreambuf_iterator<char>(file),
				std::istreambuf_iterator<char>());
			return file.good() || file.eof();
		}

		bool ScenesSaveTransactionally()
		{
			const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
			const std::filesystem::path directory =
				std::filesystem::temp_directory_path() /
				("ludus_scene_serializer_" + std::to_string(suffix));
			std::error_code fileError;
			if (!std::filesystem::create_directory(directory, fileError) || fileError)
				return false;

			struct Cleanup
			{
				std::filesystem::path path;
				~Cleanup()
				{
					std::error_code ignored;
					std::filesystem::remove_all(path, ignored);
				}
			} cleanup{ directory };

			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			Ludus::Scene source;
			const ECS::Entity entity = source.CreateEntity("entity", "Entity");
			Components::Transform transform;
			transform.position = { 1.0f, 2.0f, 3.0f };
			source.GetWorld().AddComponent(entity, transform);

			const std::filesystem::path destination = directory / "scene with spaces.lscene";
			std::vector<std::string> saveErrors;
			if (!Ludus::SceneSerializer::Save(
				destination, source, components, saveErrors))
				return false;

			std::string first;
			if (!ReadBinaryFile(destination, first) || first.find("\r\n") == std::string::npos)
				return false;

			Assets::AssetManager loadedAssets;
			Ludus::Scene loaded;
			std::vector<Ludus::SceneLoadError> loadErrors;
			if (!Ludus::SceneLoader::Load(
				destination.string(), loaded, loadedAssets, components, loadErrors))
				return false;

			source.GetWorld().GetComponent<Components::Transform>(entity).position.x = 9.0f;
			if (!Ludus::SceneSerializer::Save(
				destination, source, components, saveErrors))
				return false;

			std::string replaced;
			if (!ReadBinaryFile(destination, replaced) || replaced == first)
				return false;

			Ludus::Scene invalid;
			invalid.CreateEntity("bad id", "Invalid");
			if (Ludus::SceneSerializer::Save(
				destination, invalid, components, saveErrors))
				return false;

			std::string afterFailure;
			if (!ReadBinaryFile(destination, afterFailure) || afterFailure != replaced)
				return false;

			size_t fileCount = 0;
			for (const std::filesystem::directory_entry& entry :
				std::filesystem::directory_iterator(directory))
			{
				if (entry.path() != destination)
					return false;
				++fileCount;
			}

			std::vector<std::string> emptyPathErrors;
			return fileCount == 1 &&
				!Ludus::SceneSerializer::Save(
					{}, source, components, emptyPathErrors) &&
				!emptyPathErrors.empty();
		}

		bool RotationBoundariesRoundTrip()
		{
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			Ludus::Scene source;

			const std::array<std::pair<const char*, glm::quat>, 3> rotations
			{
				std::pair{ "x90", glm::angleAxis(glm::radians(90.0f), glm::vec3{ 1.0f, 0.0f, 0.0f }) },
				std::pair{ "y180", glm::angleAxis(glm::radians(180.0f), glm::vec3{ 0.0f, 1.0f, 0.0f }) },
				std::pair{ "z-negative", glm::angleAxis(glm::radians(-45.0f), glm::vec3{ 0.0f, 0.0f, 1.0f }) }
			};
			for (const auto& [id, rotation] : rotations)
			{
				const ECS::Entity entity = source.CreateEntity(id, id);
				Components::Transform transform;
				transform.rotation = rotation;
				source.GetWorld().AddComponent(entity, transform);
			}

			std::string text;
			std::vector<std::string> serializationErrors;
			if (!Ludus::SceneSerializer::Serialize(
				source, components, text, serializationErrors))
				return false;

			Assets::AssetManager assets;
			Ludus::Scene loaded;
			std::vector<Ludus::SceneLoadError> loadErrors;
			if (!Ludus::SceneLoader::LoadText(
				text, "rotation_round_trip.lscene", loaded, assets, components, loadErrors))
				return false;

			for (const auto& [id, rotation] : rotations)
			{
				const auto* result = loaded.GetWorld().TryGetComponent<Components::Transform>(
					loaded.FindEntity(id));
				if (!result || std::abs(glm::dot(
					glm::normalize(rotation), glm::normalize(result->rotation))) < 0.9999f)
					return false;
			}
			return true;
		}

		bool SceneRoundTrips()
		{
			Assets::AssetManager sourceAssets;
			Ludus::Scene source;
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			RegisterRotatorSceneComponent(components);

			std::vector<Ludus::SceneLoadError> loadErrors;
			if (!Ludus::SceneLoader::Load(
				"assets/scenes/maxwell.lscene",
				source,
				sourceAssets,
				components,
				loadErrors))
				return false;

			std::string serialized;
			std::vector<std::string> serializationErrors;
			if (!Ludus::SceneSerializer::Serialize(
				source, components, serialized, serializationErrors) ||
				serialized.find("\r\n") == std::string::npos)
				return false;
			std::string repeated;
			if (!Ludus::SceneSerializer::Serialize(
				source, components, repeated, serializationErrors) ||
				repeated != serialized)
				return false;

			Assets::AssetManager loadedAssets;
			Ludus::Scene loaded;
			loadErrors.clear();
			if (!Ludus::SceneLoader::LoadText(
				serialized,
				"round_trip.lscene",
				loaded,
				loadedAssets,
				components,
				loadErrors))
				return false;

			for (const std::string_view id : { "maxwell_left", "maxwell_right" })
			{
				const ECS::Entity sourceEntity = source.FindEntity(id);
				const ECS::Entity loadedEntity = loaded.FindEntity(id);
				const auto* sourceTransform =
					source.GetWorld().TryGetComponent<Components::Transform>(sourceEntity);
				const auto* loadedTransform =
					loaded.GetWorld().TryGetComponent<Components::Transform>(loadedEntity);
				const auto* sourceRenderable =
					source.GetWorld().TryGetComponent<Components::Renderable>(sourceEntity);
				const auto* loadedRenderable =
					loaded.GetWorld().TryGetComponent<Components::Renderable>(loadedEntity);
				const auto* sourceRotator = source.GetWorld().TryGetComponent<Rotator>(sourceEntity);
				const auto* loadedRotator = loaded.GetWorld().TryGetComponent<Rotator>(loadedEntity);

				if (!sourceEntity.IsValid() || !loadedEntity.IsValid() ||
					!sourceTransform || !loadedTransform ||
					!sourceRenderable || !loadedRenderable ||
					!sourceRotator || !loadedRotator ||
					source.GetEntityName(id) != loaded.GetEntityName(id) ||
					!NearlyEqual(sourceTransform->position.x, loadedTransform->position.x) ||
					!NearlyEqual(sourceTransform->position.y, loadedTransform->position.y) ||
					!NearlyEqual(sourceTransform->position.z, loadedTransform->position.z) ||
					!NearlyEqual(sourceTransform->scale.x, loadedTransform->scale.x) ||
					!NearlyEqual(sourceTransform->scale.y, loadedTransform->scale.y) ||
					!NearlyEqual(sourceTransform->scale.z, loadedTransform->scale.z) ||
					!NearlyEqual(sourceTransform->rotation.w, loadedTransform->rotation.w) ||
					!NearlyEqual(sourceTransform->rotation.x, loadedTransform->rotation.x) ||
					!NearlyEqual(sourceTransform->rotation.y, loadedTransform->rotation.y) ||
					!NearlyEqual(sourceTransform->rotation.z, loadedTransform->rotation.z) ||
					sourceRenderable->visible != loadedRenderable->visible ||
					static_cast<bool>(sourceRenderable->materialOverride) !=
						static_cast<bool>(loadedRenderable->materialOverride) ||
					!NearlyEqual(sourceRotator->axis.x, loadedRotator->axis.x) ||
					!NearlyEqual(sourceRotator->axis.y, loadedRotator->axis.y) ||
					!NearlyEqual(sourceRotator->axis.z, loadedRotator->axis.z) ||
					!NearlyEqual(sourceRotator->radiansPerSecond,
						loadedRotator->radiansPerSecond))
					return false;
			}
			return true;
		}

		bool HasLoadError(
			const char* path,
			std::string_view expectedMessage,
			bool makeSceneNonEmpty = false,
			bool requireEmptyAfterFailure = false)
		{
			Assets::AssetManager assets;
			Ludus::Scene scene;
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			RegisterRotatorSceneComponent(components);

			if (makeSceneNonEmpty)
				scene.CreateEntity("existing", "Existing");

			std::vector<Ludus::SceneLoadError> errors;
			if (Ludus::SceneLoader::Load(path, scene, assets, components, errors))
				return false;

			const bool hasError = std::ranges::any_of(errors,
				[&](const Ludus::SceneLoadError& error)
				{
					return error.path == path && error.message == expectedMessage;
				});
			return hasError &&
				(!requireEmptyAfterFailure || scene.GetWorld().GetEntityCount() == 0);
		}
	}

	bool RunSceneLoaderTests()
	{
		Assets::AssetManager assets;
		Ludus::Scene scene;
		Ludus::SceneComponentRegistry components;
		Ludus::RegisterBuiltInSceneComponents(components);
		RegisterRotatorSceneComponent(components);

		std::vector<Ludus::SceneLoadError> errors;
		if (!Ludus::SceneLoader::Load(
			"assets/scenes/maxwell.lscene",
			scene,
			assets,
			components,
			errors))
		{
			return false;
		}

		const ECS::Entity left = scene.FindEntity("maxwell_left");
		const ECS::Entity right = scene.FindEntity("maxwell_right");
		const bool loadedSceneValid = errors.empty() &&
			left.IsValid() &&
			right.IsValid() &&
			scene.GetEntityName("maxwell_left") == "Maxwell Left" &&
			scene.GetWorld().HasComponent<Components::Transform>(left) &&
			scene.GetWorld().HasComponent<Components::Renderable>(left) &&
			scene.GetWorld().HasComponent<Rotator>(left) &&
			 scene.GetWorld().HasComponent<Components::Transform>(right) &&
			 scene.GetWorld().HasComponent<Components::Renderable>(right) &&
			 scene.GetWorld().HasComponent<Rotator>(right);

		if (!loadedSceneValid)
			return false;

		scene.GetWorld().RemoveEntity(left);
		if (scene.FindEntity("maxwell_left").IsValid() ||
			!scene.GetEntityName("maxwell_left").empty())
			return false;

		const ECS::Entity replacement = scene.CreateEntity("maxwell_left", "Replacement");
		if (!replacement.IsValid() || !scene.RemoveEntity("maxwell_left") ||
			scene.GetWorld().IsEntityAlive(replacement) ||
			scene.FindEntity("maxwell_left").IsValid() ||
			!scene.GetEntityName("maxwell_left").empty())
			return false;

		return WriterBoundariesAreValidated() &&
			 SerializerBoundariesAreValidated() &&
			 EmptyAndDeadScenesSerialize() &&
			 ScenesSaveTransactionally() &&
			 RotationBoundariesRoundTrip() &&
			 SceneRoundTrips() &&
			 ComponentOrderIsIndependent() &&
			 HasLoadError(
				 "assets/scenes/tests/unknown_component.lscene",
				 "unknown component 'MissingComponent'") &&
			 HasLoadError(
				 "assets/scenes/tests/unknown_asset.lscene",
				 "unknown mesh 'missing_mesh'") &&
			 HasLoadError(
				 "assets/scenes/tests/duplicate_entity.lscene",
				 "duplicate key 'same_entity'") &&
			 HasLoadError(
				 "assets/scenes/tests/invalid_component_property.lscene",
				 "unknown component field 'wrong_field'") &&
			 HasLoadError(
				 "assets/scenes/tests/valid_empty.lscene",
				 "scene must be empty before loading",
				 true) &&
			 HasLoadError(
				 "assets/scenes/tests/float_overflow.lscene",
				 "field 'position' requires three numbers") &&
			 HasLoadError(
				 "assets/scenes/tests/zero_rotator.lscene",
				 "Rotator.axis must not be zero") &&
			 HasLoadError(
				 "assets/scenes/tests/missing_material.lscene",
				 "Renderable requires material_override because mesh 'maxwell' has surfaces without materials",
				 false,
				 true) &&
			 HasLoadError(
				 "assets/scenes/tests/partial_entity.lscene",
				 "unknown component 'ZInvalid'",
				 false,
				 true);
	}
}
