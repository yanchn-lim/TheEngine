#include "lscene_parser.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <optional>

namespace Ludus::Serialization
{
	namespace
	{
		void AddError(LSceneParseResult& result, size_t line, size_t column, std::string message)
		{
			result.errors.push_back({ std::move(message), { line, column } });
		}

		std::string_view Trim(std::string_view text)
		{
			while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
				text.remove_prefix(1);
			while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
				text.remove_suffix(1);
			return text;
		}

		std::string_view TrimRight(std::string_view text)
		{
			while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
				text.remove_suffix(1);
			return text;
		}

		std::string_view RemoveComment(std::string_view text)
		{
			bool quoted = false;
			bool escaped = false;
			for (size_t index = 0; index < text.size(); ++index)
			{
				const char character = text[index];
				if (escaped)
				{
					escaped = false;
					continue;
				}
				if (quoted && character == '\\')
				{
					escaped = true;
					continue;
				}
				if (character == '"')
					quoted = !quoted;
				else if (character == '#' && !quoted)
					return text.substr(0, index);
			}
			return text;
		}

		bool IsIdentifier(std::string_view text)
		{
			if (text.empty() || (text.front() >= '0' && text.front() <= '9'))
				return false;

			for (const char character : text)
			{
				const bool valid =
					(character >= 'a' && character <= 'z') ||
					(character >= 'A' && character <= 'Z') ||
					(character >= '0' && character <= '9') ||
					character == '_' || character == '-';
				if (!valid)
					return false;
			}
			return true;
		}

		std::optional<std::string> ParseQuotedString(
			std::string_view text,
			LSceneParseResult& result,
			SourceLocation location)
		{
			if (text.size() < 2 || text.front() != '"')
				return std::nullopt;

			std::string value;
			bool escaped = false;
			for (size_t index = 1; index < text.size(); ++index)
			{
				const char character = text[index];
				if (escaped)
				{
					switch (character)
					{
					case '"': value.push_back('"'); break;
					case '\\': value.push_back('\\'); break;
					case 'n': value.push_back('\n'); break;
					case 't': value.push_back('\t'); break;
					default:
						AddError(result, location.line, location.column + index, "unsupported escape sequence");
						return std::nullopt;
					}
					escaped = false;
					continue;
				}

				if (character == '\\')
				{
					escaped = true;
					continue;
				}

				if (character == '"')
				{
					if (!Trim(text.substr(index + 1)).empty())
					{
						AddError(result, location.line, location.column + index + 1, "unexpected text after string");
						return std::nullopt;
					}
					return value;
				}

				value.push_back(character);
			}

			AddError(result, location.line, location.column, "unterminated quoted string");
			return std::nullopt;
		}

		std::optional<LSceneValue> ParseValue(
			std::string_view text,
			LSceneParseResult& result,
			SourceLocation location,
			bool allowArray = true);

		std::optional<LSceneValue> ParseArray(
			std::string_view text,
			LSceneParseResult& result,
			SourceLocation location)
		{
			if (text.size() < 2 || text.back() != ']')
			{
				AddError(result, location.line, location.column, "array is missing closing ']'");
				return std::nullopt;
			}

			std::string_view contents = Trim(text.substr(1, text.size() - 2));
			if (contents.empty())
				return LSceneValue::ArrayValue({}, location);

			LSceneValue::Array values;
			size_t start = 0;
			bool quoted = false;
			bool escaped = false;
			for (size_t index = 0; index <= contents.size(); ++index)
			{
				const bool atEnd = index == contents.size();
				const char character = atEnd ? ',' : contents[index];
				if (atEnd && (quoted || escaped))
				{
					AddError(result, location.line, location.column + index,
						"unterminated quoted string in array");
					return std::nullopt;
				}
				if (!atEnd && escaped)
				{
					escaped = false;
					continue;
				}
				if (!atEnd && quoted && character == '\\')
				{
					escaped = true;
					continue;
				}
				if (!atEnd && character == '"')
				{
					quoted = !quoted;
					continue;
				}
				if (!atEnd && character == '[')
				{
					AddError(result, location.line, location.column + index + 1, "nested arrays are not supported");
					return std::nullopt;
				}
				if (character != ',' || quoted)
					continue;

				const std::string_view element = Trim(contents.substr(start, index - start));
				if (element.empty())
				{
					AddError(result, location.line, location.column + start + 1, "array element is empty");
					return std::nullopt;
				}

				auto value = ParseValue(element, result, { location.line, location.column + start + 1 }, false);
				if (!value)
					return std::nullopt;
				values.push_back(std::move(*value));
				start = index + 1;
			}

			return LSceneValue::ArrayValue(std::move(values), location);
		}

		std::optional<LSceneValue> ParseValue(
			std::string_view text,
			LSceneParseResult& result,
			SourceLocation location,
			bool allowArray)
		{
			text = Trim(text);
			if (text.empty())
			{
				AddError(result, location.line, location.column, "property value is missing");
				return std::nullopt;
			}

			if (text.front() == '"')
			{
				auto value = ParseQuotedString(text, result, location);
				return value ? std::optional(LSceneValue::String(std::move(*value), location)) : std::nullopt;
			}

			if (text.front() == '[')
			{
				if (!allowArray)
				{
					AddError(result, location.line, location.column, "nested arrays are not supported");
					return std::nullopt;
				}
				return ParseArray(text, result, location);
			}

			if (text == "true") return LSceneValue::Boolean(true, location);
			if (text == "false") return LSceneValue::Boolean(false, location);

			const bool floating = text.find_first_of(".eE") != std::string_view::npos;
			if (!floating)
			{
				int64_t integer = 0;
				const auto parsed = std::from_chars(text.data(), text.data() + text.size(), integer);
				if (parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size())
					return LSceneValue::Integer(integer, location);
			}
			else
			{
				std::string copy(text);
				char* end = nullptr;
				errno = 0;
				const double number = std::strtod(copy.c_str(), &end);
				if (end == copy.c_str() + copy.size() &&
					errno != ERANGE && std::isfinite(number))
					return LSceneValue::Float(number, location);
			}

			if (IsIdentifier(text))
				return LSceneValue::String(std::string(text), location);

			AddError(result, location.line, location.column, "invalid value");
			return std::nullopt;
		}

		bool InsertValue(
			LSceneParseResult& result,
			LSceneValue& parent,
			std::string key,
			LSceneValue value,
			SourceLocation location)
		{
			LSceneValue::Object* object = parent.TryGetObject();
			if (!object)
			{
				AddError(result, location.line, location.column, "parent is not a block");
				return false;
			}
			if (object->contains(key))
			{
				AddError(result, location.line, location.column, "duplicate key '" + key + "'");
				return false;
			}
			object->emplace(std::move(key), std::move(value));
			return true;
		}
	}

	LSceneParseResult LSceneParser::Parse(std::string_view source) const
	{
		LSceneParseResult result;
		// each index is a tab depth. pointers remain stable because Object
		// nodes are stored in std::map.
		std::vector<LSceneValue*> levels{ &result.root };
		bool hasDeclaration = false;
		size_t lineNumber = 0;

		while (!source.empty())
		{
			++lineNumber;
			const size_t newline = source.find('\n');
			std::string_view line = newline == std::string_view::npos ? source : source.substr(0, newline);
			source = newline == std::string_view::npos ? std::string_view{} : source.substr(newline + 1);
			line = TrimRight(RemoveComment(line));
			if (line.empty())
				continue;

			// the first content line declares the resource type and name.
			if (!hasDeclaration)
			{
				const size_t separator = line.find_first_of(" \t");
				const std::string_view type = line.substr(0, separator);
				if (separator == std::string_view::npos || !IsIdentifier(type))
				{
					AddError(result, lineNumber, 1, "expected resource declaration");
					return result;
				}
				const std::string_view nameText = Trim(line.substr(separator));
				if (nameText.empty() || nameText.front() != '"')
				{
					AddError(result, lineNumber, separator + 2, "resource name must be a quoted string");
					return result;
				}
				auto name = ParseQuotedString(nameText, result, { lineNumber, separator + 2 });
				if (!name)
					return result;
				result.resourceType = std::string(type);
				result.resourceName = std::move(*name);
				hasDeclaration = true;
				continue;
			}

			// one leading tab selects one object nesting level.
			size_t indentation = 0;
			while (indentation < line.size() && line[indentation] == '\t')
				++indentation;
			if (indentation < line.size() && line[indentation] == ' ')
			{
				AddError(result, lineNumber, indentation + 1, "leading spaces are not allowed");
				continue;
			}

			std::string_view content = Trim(line.substr(indentation));
			if (indentation >= levels.size())
			{
				AddError(result, lineNumber, 1, "indentation increased without a parent block");
				continue;
			}

			LSceneValue* parent = levels[indentation];
			levels.resize(indentation + 1);
			const size_t colon = content.find(':');
			if (colon == std::string_view::npos)
			{
				if (!IsIdentifier(content))
				{
					AddError(result, lineNumber, indentation + 1, "invalid block name");
					continue;
				}
				LSceneValue block = LSceneValue::ObjectValue({}, { lineNumber, indentation + 1 });
				if (!InsertValue(result, *parent, std::string(content), std::move(block), { lineNumber, indentation + 1 }))
					continue;
				LSceneValue* inserted = parent->TryGetObject() ? &parent->TryGetObject()->find(std::string(content))->second : nullptr;
				levels.push_back(inserted);
				continue;
			}

			const std::string_view key = Trim(content.substr(0, colon));
			if (!IsIdentifier(key))
			{
				AddError(result, lineNumber, indentation + 1, "invalid property name");
				continue;
			}
			const std::string_view valueText = Trim(content.substr(colon + 1));
			auto value = ParseValue(valueText, result, { lineNumber, indentation + colon + 2 });
			if (!value)
				continue;
			if (!InsertValue(result, *parent, std::string(key), std::move(*value), { lineNumber, indentation + 1 }))
				continue;

		}

		if (!hasDeclaration)
			AddError(result, 1, 1, "expected resource declaration");

		return result;
	}
}
