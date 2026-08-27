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
#include "editor/editor_command_history.hpp"
#include "rotator.hpp"
#include "rotator_system.hpp"
#include "scene/scene.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/scene_loader.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/scene_value_reader.hpp"
#include "scene/system_registry.hpp"
#include "serialization/lscene_parser.hpp"
#include "serialization/lscene_writer.hpp"

namespace Tests
{
	namespace
	{
		struct UnregisteredComponent
		{
		};

		class ConfiguredSystem final : public Ludus::ECS::ISystem
		{
		public:
			static constexpr Ludus::ECS::SystemPhase Phase = Ludus::ECS::SystemPhase::UPDATE;
			static constexpr int Order = 200;

			explicit ConfiguredSystem(glm::vec3 gravity) : gravity(gravity) {}

			glm::vec3 gravity;
		};

		const Ludus::SystemRegistry& TestSystems()
		{
			static const Ludus::SystemRegistry systems = []
			{
				Ludus::SystemRegistry registry;
				registry.Register<RotatorSystem>();
				registry.Register(
					"configured",
					[](const Ludus::Serialization::LSceneValue& config,
						std::vector<Ludus::SceneLoadError>& errors)
					{
						const auto* fields = Ludus::SceneValues::RequireObject(
							config, "configured config must be a block", errors);
						if (!fields || !Ludus::SceneValues::ValidateFields(
							*fields, { "gravity" }, "configured config", errors))
							return false;
						glm::vec3 gravity;
						return Ludus::SceneValues::OptionalVec3(
							*fields, "gravity", { 0.0f, -9.81f, 0.0f }, gravity, errors);
					},
					[](Ludus::ECS::World& world, const Ludus::Serialization::LSceneValue& config)
					{
						glm::vec3 gravity;
						std::vector<Ludus::SceneLoadError> errors;
						Ludus::SceneValues::OptionalVec3(
							*config.TryGetObject(),
							"gravity",
							{ 0.0f, -9.81f, 0.0f },
							gravity,
							errors);
						world.AddSystem<ConfiguredSystem>(gravity);
					});
				return registry;
			}();
			return systems;
		}

		bool LoadOrderedComponents(
			const char* path,
			Ludus::Components::Transform& transform,
			Rotator& rotator)
		{
			Ludus::Assets::AssetManager assets;
			Ludus::Scene scene;
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			RegisterRotatorSceneComponent(components);

			std::vector<Ludus::SceneLoadError> errors;
			if (!Ludus::SceneLoader::Load(
				path, scene, assets, components, TestSystems(), errors))
				return false;

			const Ludus::ECS::Entity entity = scene.FindEntity("test_entity");
			const Ludus::Components::Transform* loadedTransform =
				scene.GetWorld().TryGetComponent<Ludus::Components::Transform>(entity);
			const Rotator* loadedRotator = scene.GetWorld().TryGetComponent<Rotator>(entity);
			if (!loadedTransform || !loadedRotator)
				return false;

			transform = *loadedTransform;
			rotator = *loadedRotator;
			return true;
		}

		bool ComponentOrderIsIndependent()
		{
			Ludus::Components::Transform firstTransform;
			Ludus::Components::Transform secondTransform;
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

		Ludus::Serialization::LSceneValue MakeWriterRoot()
		{
			Ludus::Serialization::LSceneValue::Object root;
			root.emplace("scene", Ludus::Serialization::LSceneValue::String("Test", {}));
			root.emplace("version", Ludus::Serialization::LSceneValue::Integer(1, {}));
			root.emplace("entities", Ludus::Serialization::LSceneValue::ObjectValue());
			return Ludus::Serialization::LSceneValue::ObjectValue(std::move(root));
		}

		Ludus::Serialization::LSceneValue::Object& WriterEntities(
			Ludus::Serialization::LSceneValue& root)
		{
			return *root.TryGetObject()->find("entities")->second.TryGetObject();
		}

		bool WriterRejects(Ludus::Serialization::LSceneValue root)
		{
			std::string output;
			std::string error;
			return !Ludus::Serialization::LSceneWriter::Write(root, output, error) &&
				!error.empty();
		}

		bool WriterBoundariesAreValidated()
		{
			Ludus::Serialization::LSceneValue invalidVersion = MakeWriterRoot();
			invalidVersion.TryGetObject()->insert_or_assign(
				"version", Ludus::Serialization::LSceneValue::Integer(2, {}));

			Ludus::Serialization::LSceneValue missingEntities = MakeWriterRoot();
			missingEntities.TryGetObject()->erase("entities");

			Ludus::Serialization::LSceneValue invalidIdentifier = MakeWriterRoot();
			WriterEntities(invalidIdentifier).emplace(
				"bad id", Ludus::Serialization::LSceneValue::ObjectValue());

			Ludus::Serialization::LSceneValue emptyArray = MakeWriterRoot();
			Ludus::Serialization::LSceneValue::Object emptyArrayEntity;
			emptyArrayEntity.emplace("value",
				Ludus::Serialization::LSceneValue::ArrayValue({}, {}));
			WriterEntities(emptyArray).emplace(
				"entity", Ludus::Serialization::LSceneValue::ObjectValue(std::move(emptyArrayEntity)));

			Ludus::Serialization::LSceneValue nestedArray = MakeWriterRoot();
			Ludus::Serialization::LSceneValue::Array inner;
			inner.push_back(Ludus::Serialization::LSceneValue::Float(1.0, {}));
			Ludus::Serialization::LSceneValue::Array outer;
			outer.push_back(Ludus::Serialization::LSceneValue::ArrayValue(std::move(inner), {}));
			Ludus::Serialization::LSceneValue::Object nestedArrayEntity;
			nestedArrayEntity.emplace("value",
				Ludus::Serialization::LSceneValue::ArrayValue(std::move(outer), {}));
			WriterEntities(nestedArray).emplace(
				"entity", Ludus::Serialization::LSceneValue::ObjectValue(std::move(nestedArrayEntity)));

			Ludus::Serialization::LSceneValue nonFinite = MakeWriterRoot();
			Ludus::Serialization::LSceneValue::Object nonFiniteEntity;
			nonFiniteEntity.emplace("value", Ludus::Serialization::LSceneValue::Float(
				std::numeric_limits<double>::infinity(), {}));
			WriterEntities(nonFinite).emplace(
				"entity", Ludus::Serialization::LSceneValue::ObjectValue(std::move(nonFiniteEntity)));

			Ludus::Serialization::LSceneValue controlCharacter = MakeWriterRoot();
			controlCharacter.TryGetObject()->insert_or_assign(
				"scene", Ludus::Serialization::LSceneValue::String("Bad\rName", {}));

			Ludus::Serialization::LSceneValue escaped = MakeWriterRoot();
			const std::string escapedName = "Quote \" slash \\ line\n tab\t";
			escaped.TryGetObject()->insert_or_assign(
				"scene", Ludus::Serialization::LSceneValue::String(escapedName, {}));
			std::string text;
			std::string error;
			if (!Ludus::Serialization::LSceneWriter::Write(escaped, text, error))
				return false;
			const Ludus::Serialization::LSceneParseResult parsed =
				Ludus::Serialization::LSceneParser{}.Parse(text);
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
				scene, components, TestSystems(), output, errors) &&
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
			const Ludus::ECS::Entity foreignEntity = foreignMesh.CreateEntity("entity", "Entity");
			foreignMesh.GetWorld().AddComponent(
				foreignEntity,
				Ludus::Components::Renderable{ Ludus::Assets::MeshHandle{ 99 }, {}, true });

			Ludus::Scene ambiguousMesh;
			Ludus::SceneAssetContext ambiguousAssets;
			ambiguousAssets.meshes.emplace("first", Ludus::Assets::MeshHandle{ 7 });
			ambiguousAssets.meshes.emplace("second", Ludus::Assets::MeshHandle{ 7 });
			ambiguousMesh.SetSerializationData(
				"Ambiguous",
				Ludus::Serialization::LSceneValue::ObjectValue(),
				std::move(ambiguousAssets));
			const Ludus::ECS::Entity ambiguousEntity =
				ambiguousMesh.CreateEntity("entity", "Entity");
			ambiguousMesh.GetWorld().AddComponent(
				ambiguousEntity,
				Ludus::Components::Renderable{ Ludus::Assets::MeshHandle{ 7 }, {}, true });

			Ludus::Scene invalidFloat;
			const Ludus::ECS::Entity invalidFloatEntity =
				invalidFloat.CreateEntity("entity", "Entity");
			Ludus::Components::Transform invalidTransform;
			invalidTransform.position.x = std::numeric_limits<float>::infinity();
			invalidFloat.GetWorld().AddComponent(invalidFloatEntity, invalidTransform);

			Ludus::Scene invalidQuaternion;
			const Ludus::ECS::Entity invalidQuaternionEntity =
				invalidQuaternion.CreateEntity("entity", "Entity");
			Ludus::Components::Transform zeroRotation;
			zeroRotation.rotation = glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f };
			invalidQuaternion.GetWorld().AddComponent(
				invalidQuaternionEntity, zeroRotation);

			Ludus::Scene invalidRotator;
			const Ludus::ECS::Entity invalidRotatorEntity =
				invalidRotator.CreateEntity("entity", "Entity");
			invalidRotator.GetWorld().AddComponent(
				invalidRotatorEntity,
				Rotator{ {}, 1.0f });

			Ludus::Scene unregisteredComponent;
			const Ludus::ECS::Entity unregisteredEntity =
				unregisteredComponent.CreateEntity("entity", "Entity");
			unregisteredComponent.GetWorld().AddComponent(
				unregisteredEntity,
				UnregisteredComponent{});

			Ludus::Scene missingMaterial;
			Ludus::SceneAssetContext missingMaterialAssets;
			missingMaterialAssets.meshes.emplace("mesh", Ludus::Assets::MeshHandle{ 8 });
			missingMaterialAssets.meshHasDefaultMaterials.emplace("mesh", false);
			missingMaterial.SetSerializationData(
				"Missing Material",
				Ludus::Serialization::LSceneValue::ObjectValue(),
				std::move(missingMaterialAssets));
			const Ludus::ECS::Entity missingMaterialEntity =
				missingMaterial.CreateEntity("entity", "Entity");
			missingMaterial.GetWorld().AddComponent(
				missingMaterialEntity,
				Ludus::Components::Renderable{ Ludus::Assets::MeshHandle{ 8 }, {}, true });

			Ludus::Scene duplicateSystems;
			duplicateSystems.SetSystems({
				{ "rotator", true, Ludus::Serialization::LSceneValue::ObjectValue() },
				{ "rotator", false, Ludus::Serialization::LSceneValue::ObjectValue() }
			});

			Ludus::Scene unknownSystem;
			unknownSystem.SetSystems({
				{ "missing", true, Ludus::Serialization::LSceneValue::ObjectValue() }
			});

			Ludus::Scene invalidSystemConfig;
			invalidSystemConfig.SetSystems({
				{ "rotator", true, Ludus::Serialization::LSceneValue::Float(1.0, {}) }
			});

			return SerializationFails(invalidId, components) &&
				SerializationFails(invalidName, components) &&
				SerializationFails(foreignMesh, components) &&
				SerializationFails(ambiguousMesh, components) &&
				SerializationFails(invalidFloat, components) &&
				SerializationFails(invalidQuaternion, components) &&
				SerializationFails(invalidRotator, components) &&
				SerializationFails(unregisteredComponent, components) &&
				SerializationFails(missingMaterial, components) &&
				SerializationFails(duplicateSystems, components) &&
				SerializationFails(unknownSystem, components) &&
				SerializationFails(invalidSystemConfig, components);
		}

		bool SystemTextFails(
			std::string_view source,
			std::string_view expectedMessage)
		{
			Ludus::Assets::AssetManager assets;
			Ludus::Scene scene;
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			std::vector<Ludus::SceneLoadError> errors;
			if (Ludus::SceneLoader::LoadText(
				source,
				"system_validation.lscene",
				scene,
				assets,
				components,
				TestSystems(),
				errors))
			{
				return false;
			}

			return scene.GetWorld().GetEntityCount() == 0 &&
				scene.GetWorld().GetSystemCount() == 0 &&
				scene.GetSystems().empty() &&
				std::ranges::any_of(
					errors,
					[expectedMessage](const Ludus::SceneLoadError& error)
					{
						return error.message == expectedMessage;
					});
		}

		bool ComponentUpdatesAreTransactional()
		{
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			Ludus::Scene scene;
			const Ludus::ECS::Entity entity = scene.CreateEntity("entity", "Entity");
			Ludus::Components::Transform transform;
			transform.position = { 1.0f, 2.0f, 3.0f };
			scene.GetWorld().AddComponent(entity, transform);

			Ludus::Serialization::LSceneValue::Object values;
			std::vector<std::string> saveErrors;
			if (!components.SaveComponents(
				scene.GetAssetContext(), scene.GetWorld(), entity, values, saveErrors))
			{
				return false;
			}

			auto transformValue = values.find("Transform");
			auto* position = transformValue != values.end()
				? transformValue->second.TryGetObject()->find("position")->second.TryGetArray()
				: nullptr;
			if (!position || position->size() != 3)
				return false;
			(*position)[0] = Ludus::Serialization::LSceneValue::Float(4.0, {});

			std::vector<Ludus::SceneLoadError> updateErrors;
			if (!components.Update(
				"Transform",
				transformValue->second,
				scene.GetAssetContext(),
				scene.GetWorld(),
				entity,
				updateErrors) ||
				scene.GetWorld().GetComponent<Ludus::Components::Transform>(entity).position.x != 4.0f)
			{
				return false;
			}

			position->pop_back();
			updateErrors.clear();
			return !components.Update(
				"Transform",
				transformValue->second,
				scene.GetAssetContext(),
				scene.GetWorld(),
				entity,
				updateErrors) &&
				!updateErrors.empty() &&
				scene.GetWorld().GetComponent<Ludus::Components::Transform>(entity).position.x == 4.0f;
		}

		bool EditorCommandsUndoAndRedo()
		{
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			Ludus::Scene scene;
			const Ludus::ECS::Entity entity = scene.CreateEntity("entity", "Entity");
			Ludus::Components::Transform transform;
			transform.position.x = 1.0f;
			scene.GetWorld().AddComponent(entity, transform);

			Ludus::Serialization::LSceneValue before = Ludus::Serialization::LSceneValue::ObjectValue();
			Ludus::Serialization::LSceneValue after = Ludus::Serialization::LSceneValue::ObjectValue();
			std::vector<std::string> saveErrors;
			Ludus::Serialization::LSceneValue::Object values;
			if (!components.SaveComponents(
				scene.GetAssetContext(), scene.GetWorld(), entity, values, saveErrors))
			{
				return false;
			}
			before = values.at("Transform");

			scene.GetWorld().GetComponent<Ludus::Components::Transform>(entity).position.x = 4.0f;
			values.clear();
			if (!components.SaveComponents(
				scene.GetAssetContext(), scene.GetWorld(), entity, values, saveErrors))
			{
				return false;
			}
			after = values.at("Transform");

			Ludus::Editor::EditorCommandHistory history;
			history.RecordComponentEdit("entity", "Transform", before, after);
			if (!history.CanUndo() || history.CanRedo() ||
				!history.Undo(scene, components) ||
				!NearlyEqual(
					scene.GetWorld().GetComponent<Ludus::Components::Transform>(entity).position.x,
					1.0f) ||
				!history.CanRedo() ||
				!history.Redo(scene, components) ||
				!NearlyEqual(
					scene.GetWorld().GetComponent<Ludus::Components::Transform>(entity).position.x,
					4.0f))
			{
				return false;
			}

			Ludus::Serialization::LSceneValue::Object beforeFields;
			beforeFields.emplace(
				"speed", Ludus::Serialization::LSceneValue::Float(1.0, {}));
			Ludus::Serialization::LSceneValue::Object afterFields;
			afterFields.emplace(
				"speed", Ludus::Serialization::LSceneValue::Float(2.0, {}));
			Ludus::Serialization::LSceneValue beforeConfig =
				Ludus::Serialization::LSceneValue::ObjectValue(std::move(beforeFields));
			Ludus::Serialization::LSceneValue afterConfig =
				Ludus::Serialization::LSceneValue::ObjectValue(std::move(afterFields));
			scene.SetSystems({ { "rotator", false, afterConfig } });
			history.RecordSystemEdit(
				"rotator", false, beforeConfig, false, afterConfig);

			if (!history.Undo(scene, components) || scene.GetSystems()[0].enabled)
				return false;
			const double* undoneSpeed = scene.GetSystems()[0].config.Find("speed")
				? scene.GetSystems()[0].config.Find("speed")->TryGetFloat()
				: nullptr;
			if (!undoneSpeed || !NearlyEqual(static_cast<float>(*undoneSpeed), 1.0f) ||
				!history.Redo(scene, components) || scene.GetSystems()[0].enabled)
			{
				return false;
			}

			const double* redoneSpeed = scene.GetSystems()[0].config.Find("speed")
				? scene.GetSystems()[0].config.Find("speed")->TryGetFloat()
				: nullptr;
			if (!redoneSpeed || !NearlyEqual(static_cast<float>(*redoneSpeed), 2.0f) ||
				!history.Undo(scene, components))
			{
				return false;
			}

			history.RecordSystemEdit(
				"rotator", false, beforeConfig, true, beforeConfig);
			return !history.CanRedo();
		}

		bool SystemCompositionRoundTrips()
		{
			constexpr std::string_view source =
				"scene \"Systems\"\n"
				"version: 1\n\n"
				"systems\n"
				"\tconfigured\n"
				"\t\tenabled: true\n"
				"\t\tconfig\n"
				"\t\t\tgravity: [0.0, -9.81, 0.0]\n"
				"\trotator\n"
				"\t\tenabled: false\n\n"
				"entities\n";

			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			Ludus::Assets::AssetManager assets;
			Ludus::Scene scene;
			std::vector<Ludus::SceneLoadError> loadErrors;
			if (!Ludus::SceneLoader::LoadText(
				source, "systems.lscene", scene, assets, components,
				TestSystems(), loadErrors) ||
				scene.GetSystems().size() != 2 ||
				scene.GetWorld().GetSystemCount() != 1)
			{
				return false;
			}

			std::string text;
			std::vector<std::string> serializationErrors;
			if (!Ludus::SceneSerializer::Serialize(
				scene, components, TestSystems(), text, serializationErrors) ||
				text.find("systems\r\n") == std::string::npos)
			{
				return false;
			}

			Ludus::Assets::AssetManager roundTripAssets;
			Ludus::Scene roundTrip;
			loadErrors.clear();
			if (!Ludus::SceneLoader::LoadText(
				text, "systems_round_trip.lscene", roundTrip, roundTripAssets,
				components, TestSystems(), loadErrors) ||
				roundTrip.GetSystems().size() != 2 ||
				roundTrip.GetWorld().GetSystemCount() != 1)
			{
				return false;
			}

			const auto configured = std::ranges::find(
				roundTrip.GetSystems(), "configured", &Ludus::SceneSystemDefinition::id);
			const Ludus::Serialization::LSceneValue* gravityValue =
				configured != roundTrip.GetSystems().end()
				? configured->config.Find("gravity")
				: nullptr;
			const auto* gravity = gravityValue ? gravityValue->TryGetArray() : nullptr;
			float gravityY = 0.0f;
			if (!gravity || gravity->size() != 3 ||
				!Ludus::SceneValues::FiniteFloat((*gravity)[1], gravityY) ||
				!NearlyEqual(gravityY, -9.81f))
			{
				return false;
			}

			constexpr std::string_view noSystems =
				"scene \"No Systems\"\n"
				"version: 1\n\n"
				"entities\n";
			Ludus::Scene withoutSystems;
			loadErrors.clear();
			if (!Ludus::SceneLoader::LoadText(
				noSystems, "no_systems.lscene", withoutSystems, roundTripAssets,
				components, TestSystems(), loadErrors) ||
				withoutSystems.GetWorld().GetSystemCount() != 0 ||
				!withoutSystems.GetSystems().empty())
			{
				return false;
			}

			return
				SystemTextFails(
					"scene \"Unknown\"\nversion: 1\n\nsystems\n\tmissing\n\t\tenabled: true\nentities\n",
					"unknown or unavailable system 'missing'") &&
				SystemTextFails(
					"scene \"Duplicate\"\nversion: 1\n\nsystems\n\trotator\n\t\tenabled: true\n\trotator\n\t\tenabled: false\nentities\n",
					"duplicate key 'rotator'") &&
				SystemTextFails(
					"scene \"Enabled\"\nversion: 1\n\nsystems\n\trotator\n\t\tenabled: 1\nentities\n",
					"system 'rotator' enabled must be a boolean") &&
				SystemTextFails(
					"scene \"Field\"\nversion: 1\n\nsystems\n\trotator\n\t\tenabled: true\n\t\torder: 5\nentities\n",
					"unknown system field 'order'") &&
				SystemTextFails(
					"scene \"Config\"\nversion: 1\n\nsystems\n\trotator\n\t\tenabled: true\n\t\tconfig: 1\nentities\n",
					"system 'rotator' config must be a block") &&
				SystemTextFails(
					"scene \"Disabled\"\nversion: 1\n\nsystems\n\tconfigured\n\t\tenabled: false\n\t\tconfig\n\t\t\tgravity: [0.0, 1.0]\nentities\n",
					"field 'gravity' requires three numbers");
		}

		bool EmptyAndDeadScenesSerialize()
		{
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);

			Ludus::Scene source;
			const Ludus::ECS::Entity removed = source.CreateEntity("removed", "Removed");
			source.GetWorld().RemoveEntity(removed);

			std::string text;
			std::vector<std::string> serializationErrors;
			if (!Ludus::SceneSerializer::Serialize(
				source, components, TestSystems(), text, serializationErrors))
				return false;

			Ludus::Assets::AssetManager assets;
			Ludus::Scene loaded;
			std::vector<Ludus::SceneLoadError> loadErrors;
			return Ludus::SceneLoader::LoadText(
				text,
				"empty_round_trip.lscene",
				loaded,
				assets,
				components,
				TestSystems(),
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
			const Ludus::ECS::Entity entity = source.CreateEntity("entity", "Entity");
			Ludus::Components::Transform transform;
			transform.position = { 1.0f, 2.0f, 3.0f };
			source.GetWorld().AddComponent(entity, transform);

			const std::filesystem::path destination = directory / "scene with spaces.lscene";
			std::vector<std::string> saveErrors;
			if (!Ludus::SceneSerializer::Save(
				destination, source, components, TestSystems(), saveErrors))
				return false;

			std::string first;
			if (!ReadBinaryFile(destination, first) || first.find("\r\n") == std::string::npos)
				return false;

			Ludus::Assets::AssetManager loadedAssets;
			Ludus::Scene loaded;
			std::vector<Ludus::SceneLoadError> loadErrors;
			if (!Ludus::SceneLoader::Load(
				destination.string(), loaded, loadedAssets, components,
				TestSystems(), loadErrors))
				return false;

			source.GetWorld().GetComponent<Ludus::Components::Transform>(entity).position.x = 9.0f;
			if (!Ludus::SceneSerializer::Save(
				destination, source, components, TestSystems(), saveErrors))
				return false;

			std::string replaced;
			if (!ReadBinaryFile(destination, replaced) || replaced == first)
				return false;

			Ludus::Scene invalid;
			invalid.CreateEntity("bad id", "Invalid");
			if (Ludus::SceneSerializer::Save(
				destination, invalid, components, TestSystems(), saveErrors))
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
					{}, source, components, TestSystems(), emptyPathErrors) &&
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
				const Ludus::ECS::Entity entity = source.CreateEntity(id, id);
				Ludus::Components::Transform transform;
				transform.rotation = rotation;
				source.GetWorld().AddComponent(entity, transform);
			}

			std::string text;
			std::vector<std::string> serializationErrors;
			if (!Ludus::SceneSerializer::Serialize(
				source, components, TestSystems(), text, serializationErrors))
				return false;

			Ludus::Assets::AssetManager assets;
			Ludus::Scene loaded;
			std::vector<Ludus::SceneLoadError> loadErrors;
			if (!Ludus::SceneLoader::LoadText(
				text, "rotation_round_trip.lscene", loaded, assets, components,
				TestSystems(), loadErrors))
				return false;

			for (const auto& [id, rotation] : rotations)
			{
				const auto* result = loaded.GetWorld().TryGetComponent<Ludus::Components::Transform>(
					loaded.FindEntity(id));
				if (!result || std::abs(glm::dot(
					glm::normalize(rotation), glm::normalize(result->rotation))) < 0.9999f)
					return false;
			}
			return true;
		}

		bool SceneRoundTrips()
		{
			Ludus::Assets::AssetManager sourceAssets;
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
				TestSystems(),
				loadErrors))
				return false;

			std::string serialized;
			std::vector<std::string> serializationErrors;
			if (!Ludus::SceneSerializer::Serialize(
				source, components, TestSystems(), serialized, serializationErrors) ||
				serialized.find("\r\n") == std::string::npos)
				return false;
			std::string repeated;
			if (!Ludus::SceneSerializer::Serialize(
				source, components, TestSystems(), repeated, serializationErrors) ||
				repeated != serialized)
				return false;

			Ludus::Assets::AssetManager loadedAssets;
			Ludus::Scene loaded;
			loadErrors.clear();
			if (!Ludus::SceneLoader::LoadText(
				serialized,
				"round_trip.lscene",
				loaded,
				loadedAssets,
				components,
				TestSystems(),
				loadErrors))
				return false;

			for (const std::string_view id : { "maxwell_left", "maxwell_right" })
			{
				const Ludus::ECS::Entity sourceEntity = source.FindEntity(id);
				const Ludus::ECS::Entity loadedEntity = loaded.FindEntity(id);
				const auto* sourceTransform =
					source.GetWorld().TryGetComponent<Ludus::Components::Transform>(sourceEntity);
				const auto* loadedTransform =
					loaded.GetWorld().TryGetComponent<Ludus::Components::Transform>(loadedEntity);
				const auto* sourceRenderable =
					source.GetWorld().TryGetComponent<Ludus::Components::Renderable>(sourceEntity);
				const auto* loadedRenderable =
					loaded.GetWorld().TryGetComponent<Ludus::Components::Renderable>(loadedEntity);
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
			Ludus::Assets::AssetManager assets;
			Ludus::Scene scene;
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			RegisterRotatorSceneComponent(components);

			if (makeSceneNonEmpty)
				scene.CreateEntity("existing", "Existing");

			std::vector<Ludus::SceneLoadError> errors;
			if (Ludus::SceneLoader::Load(
				path, scene, assets, components, TestSystems(), errors))
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
		Ludus::Assets::AssetManager assets;
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
			TestSystems(),
			errors))
		{
			return false;
		}

		const Ludus::ECS::Entity left = scene.FindEntity("maxwell_left");
		const Ludus::ECS::Entity right = scene.FindEntity("maxwell_right");
		const bool loadedSceneValid = errors.empty() &&
			left.IsValid() &&
			right.IsValid() &&
			scene.GetEntityName("maxwell_left") == "Maxwell Left" &&
			scene.GetWorld().HasComponent<Ludus::Components::Transform>(left) &&
			scene.GetWorld().HasComponent<Ludus::Components::Renderable>(left) &&
			scene.GetWorld().HasComponent<Rotator>(left) &&
			 scene.GetWorld().HasComponent<Ludus::Components::Transform>(right) &&
			 scene.GetWorld().HasComponent<Ludus::Components::Renderable>(right) &&
			 scene.GetWorld().HasComponent<Rotator>(right);

		if (!loadedSceneValid)
			return false;

		scene.GetWorld().RemoveEntity(left);
		if (scene.FindEntity("maxwell_left").IsValid() ||
			!scene.GetEntityName("maxwell_left").empty())
			return false;

		const Ludus::ECS::Entity replacement = scene.CreateEntity("maxwell_left", "Replacement");
		if (!replacement.IsValid() || !scene.RemoveEntity("maxwell_left") ||
			scene.GetWorld().IsEntityAlive(replacement) ||
			scene.FindEntity("maxwell_left").IsValid() ||
			!scene.GetEntityName("maxwell_left").empty())
			return false;

		return WriterBoundariesAreValidated() &&
			 SerializerBoundariesAreValidated() &&
			 ComponentUpdatesAreTransactional() &&
			 EditorCommandsUndoAndRedo() &&
			 SystemCompositionRoundTrips() &&
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
