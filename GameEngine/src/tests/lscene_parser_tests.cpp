#include "lscene_parser_tests.hpp"

#include <string_view>

#include "serialization/lscene_parser.hpp"

namespace Tests
{
	namespace
	{
		bool HasError(std::string_view source)
		{
			return !Serialization::LSceneParser{}.Parse(source);
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

		const Serialization::LSceneParseResult parsed =
			Serialization::LSceneParser{}.Parse(validScene);
		if (!parsed)
			return false;

		const Serialization::LSceneValue* scene = parsed.root.Find("scene");
		const Serialization::LSceneValue* version = parsed.root.Find("version");
		const Serialization::LSceneValue* entities = parsed.root.Find("entities");
		if (!scene || !scene->TryGetString() || *scene->TryGetString() != "Parser Test" ||
			!version || !version->TryGetInteger() || *version->TryGetInteger() != 1 ||
			!entities)
		{
			return false;
		}

		const Serialization::LSceneValue* maxwell = entities->Find("maxwell");
		const Serialization::LSceneValue* components = maxwell ? maxwell->Find("components") : nullptr;
		const Serialization::LSceneValue* transform = components ? components->Find("Transform") : nullptr;
		const Serialization::LSceneValue* position = transform ? transform->Find("position") : nullptr;
		const Serialization::LSceneValue::Array* values = position ? position->TryGetArray() : nullptr;
		if (!values || values->size() != 3 ||
			!(*values)[0].TryGetFloat() || !(*values)[1].TryGetInteger() || !(*values)[2].TryGetFloat())
		{
			return false;
		}

		constexpr std::string_view quotedArraySource =
			"scene \"Quoted Array\"\n"
			"version: 1\n"
			"value: [\"comma,inside\", \"hash#inside\"]\n";
		const Serialization::LSceneParseResult quotedArray =
			Serialization::LSceneParser{}.Parse(quotedArraySource);
		const Serialization::LSceneValue* quotedValue = quotedArray.root.Find("value");
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
			HasError("scene \"Test\"\nversion: 2\n") &&
			HasError("scene \"Test\"\n version: 1\n") &&
			HasError("scene \"Test\"\nversion: 1\n\t\tchild\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: 1\nvalue: 2\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: [1, 2\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: [[1], 2]\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: [1, \"unterminated]\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: [\"dangling\\]\n") &&
			HasError("scene \"Test\"\nversion: 1\nvalue: 1e999\n") &&
			!HasError("scene \"Test\"\nversion: 1\nvalue: 1.7976931348623157e308\n");
	}
}
