#include "lscene_parser_tests.hpp"

#include <string_view>

#include "serialization/lscene_parser.hpp"

namespace Tests
{
	namespace
	{
		bool HasError(std::string_view source)
		{
			return !Ludus::Serialization::LSceneParser{}.Parse(source);
		}
	}

	bool RunLSceneParserTests()
	{
		constexpr std::string_view validScene =
			"scene \"Parser Test\"\n"
			"version: 1\n"
			"\n"
			"# comment\n"
			"entities\n"
			"\tmaxwell\n"
			"\t\tname: \"Maxwell #1\" # inline comment\n"
			"\t\tcomponents\n"
			"\t\t\tTransform\n"
			"\t\t\t\tposition: [-0.85, 0, 0.25]\n"
			"\t\t\t\tvisible: true\n";

		const Ludus::Serialization::LSceneParseResult parsed =
			Ludus::Serialization::LSceneParser{}.Parse(validScene);
		if (!parsed)
			return false;

		const Ludus::Serialization::LSceneValue* version = parsed.root.Find("version");
		const Ludus::Serialization::LSceneValue* entities = parsed.root.Find("entities");
		if (parsed.resourceType != "scene" || parsed.resourceName != "Parser Test" ||
			!version || !version->TryGetInteger() || *version->TryGetInteger() != 1 ||
			!entities)
		{
			return false;
		}

		const auto material = Ludus::Serialization::LSceneParser{}.Parse(
			"material \"Default\"\nshader: \"assets/shaders/default.lshader\"\n");
		const auto shader = Ludus::Serialization::LSceneParser{}.Parse(
			"shader \"Default\"\nvertex: \"default.vert\"\n");
		const auto emptyArray = Ludus::Serialization::LSceneParser{}.Parse(
			"scene \"Empty Array\"\nvalue: []\n");
		if (!material || material.resourceType != "material" || material.resourceName != "Default" ||
			!shader || shader.resourceType != "shader" || shader.resourceName != "Default" ||
			!emptyArray || !emptyArray.root.Find("value") ||
			!emptyArray.root.Find("value")->TryGetArray() ||
			!emptyArray.root.Find("value")->TryGetArray()->empty())
		{
			return false;
		}

		const Ludus::Serialization::LSceneValue* maxwell = entities->Find("maxwell");
		const Ludus::Serialization::LSceneValue* components = maxwell ? maxwell->Find("components") : nullptr;
		const Ludus::Serialization::LSceneValue* transform = components ? components->Find("Transform") : nullptr;
		const Ludus::Serialization::LSceneValue* position = transform ? transform->Find("position") : nullptr;
		const Ludus::Serialization::LSceneValue::Array* values = position ? position->TryGetArray() : nullptr;
		if (!values || values->size() != 3 ||
			!(*values)[0].TryGetFloat() || !(*values)[1].TryGetInteger() || !(*values)[2].TryGetFloat())
		{
			return false;
		}

		constexpr std::string_view quotedArraySource =
			"scene \"Quoted Array\"\n"
			"version: 1\n"
			"value: [\"comma,inside\", \"hash#inside\"]\n";
		const Ludus::Serialization::LSceneParseResult quotedArray =
			Ludus::Serialization::LSceneParser{}.Parse(quotedArraySource);
		const Ludus::Serialization::LSceneValue* quotedValue = quotedArray.root.Find("value");
		const auto* quotedValues = quotedValue ? quotedValue->TryGetArray() : nullptr;
		if (!quotedArray || !quotedValues || quotedValues->size() != 2 ||
			!(*quotedValues)[0].TryGetString() || *(*quotedValues)[0].TryGetString() != "comma,inside" ||
			!(*quotedValues)[1].TryGetString() || *(*quotedValues)[1].TryGetString() != "hash#inside")
		{
			return false;
		}

		return
			HasError("version: 1\n") &&
			HasError("scene Test\nversion: 1\n") &&
			HasError("scene \"Test\"\n version: 1\n") &&
			HasError("scene \"Test\"\nversion: 1\n\t\tchild\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: 1\nvalue: 2\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: [1, 2\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: [[1], 2]\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: [1, \"unterminated]\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: [\"dangling\\]\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: 1e999\n") &&
			!HasError("scene \"Test\"\n") &&
			!HasError("scene \"Test\"\nversion: 2\n") &&
			!HasError("scene \"Test\"\nversion: 1\nvalue: 1.7976931348623157e308\n");
	}
}
