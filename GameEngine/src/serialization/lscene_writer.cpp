#include "lscene_writer.hpp"

#include <charconv>
#include <cmath>
#include <limits>
#include <utility>

namespace Ludus::Serialization
{
	namespace
	{
		void AppendIndent(std::string& output, size_t indentation)
		{
			output.append(indentation, '\t');
		}

		bool IsIdentifier(std::string_view value)
		{
			if (value.empty() || (value.front() >= '0' && value.front() <= '9'))
				return false;
			for (const char character : value)
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

		bool AppendEscapedString(
			std::string& output,
			std::string_view value,
			std::string& error)
		{
			output.push_back('"');
			for (const char character : value)
			{
				switch (character)
				{
				case '"': output += "\\\""; break;
				case '\\': output += "\\\\"; break;
				case '\n': output += "\\n"; break;
				case '\t': output += "\\t"; break;
				default:
					if (static_cast<unsigned char>(character) < 0x20)
					{
						error = "string contains an unsupported control character";
						return false;
					}
					output.push_back(character);
					break;
				}
			}
			output.push_back('"');
			return true;
		}

		bool AppendValue(
			const LSceneValue& value,
			std::string& output,
			std::string& error,
			bool allowArray = true)
		{
			if (const bool* boolean = value.TryGetBoolean())
			{
				output += *boolean ? "true" : "false";
				return true;
			}
			if (const int64_t* integer = value.TryGetInteger())
			{
				char buffer[32];
				const auto result = std::to_chars(buffer, buffer + sizeof(buffer), *integer);
				if (result.ec != std::errc{})
				{
					error = "failed to format integer";
					return false;
				}
				output.append(buffer, result.ptr);
				return true;
			}
			if (const double* floating = value.TryGetFloat())
			{
				if (!std::isfinite(*floating))
				{
					error = "scene contains a non-finite floating-point value";
					return false;
				}
				char buffer[64];
				const auto result = std::to_chars(
					buffer,
					buffer + sizeof(buffer),
					*floating,
					std::chars_format::general,
					std::numeric_limits<double>::max_digits10);
				if (result.ec != std::errc{})
				{
					error = "failed to format floating-point value";
					return false;
				}
				const std::string_view text(
					buffer,
					static_cast<size_t>(result.ptr - buffer));
				output.append(text);
				if (text.find_first_of(".eE") == std::string_view::npos)
					output += ".0";
				return true;
			}
			if (const std::string* text = value.TryGetString())
				return AppendEscapedString(output, *text, error);
			if (const LSceneValue::Array* array = value.TryGetArray())
			{
				if (!allowArray)
				{
					error = "nested arrays are not supported in scene version 1";
					return false;
				}
				if (array->empty())
				{
					error = "empty arrays are not supported in scene version 1";
					return false;
				}
				output.push_back('[');
				for (size_t index = 0; index < array->size(); ++index)
				{
					if (index != 0)
						output += ", ";
					if (!AppendValue((*array)[index], output, error, false))
						return false;
				}
				output.push_back(']');
				return true;
			}

			error = "scene property has an unsupported value";
			return false;
		}

		bool AppendObject(
			const LSceneValue::Object& object,
			size_t indentation,
			std::string& output,
			std::string& error)
		{
			for (const auto& [name, value] : object)
			{
				if (!IsIdentifier(name))
				{
					error = "invalid scene identifier '" + name + "'";
					return false;
				}
				AppendIndent(output, indentation);
				output += name;
				if (const LSceneValue::Object* child = value.TryGetObject())
				{
					output += "\r\n";
					if (!AppendObject(*child, indentation + 1, output, error))
						return false;
				}
				else
				{
					output += ": ";
					if (!AppendValue(value, output, error))
						return false;
					output += "\r\n";
				}
			}
			return true;
		}

		bool AppendRootBlock(
			std::string_view name,
			const LSceneValue* value,
			std::string& output,
			std::string& error)
		{
			if (!value)
				return true;
			const LSceneValue::Object* object = value->TryGetObject();
			if (!object)
			{
				error = "root field '" + std::string(name) + "' must be a block";
				return false;
			}

			output += name;
			output += "\r\n";
			return AppendObject(*object, 1, output, error);
		}
	}

	bool LSceneWriter::Write(
		const LSceneValue& root,
		std::string& output,
		std::string& error)
	{
		output.clear();
		error.clear();
		std::string written;

		const std::string* name = root.Find("scene")
			? root.Find("scene")->TryGetString()
			: nullptr;
		const int64_t* version = root.Find("version")
			? root.Find("version")->TryGetInteger()
			: nullptr;
		if (!name || !version)
		{
			error = "scene root requires a name and integer version";
			return false;
		}
		if (*version != 1)
		{
			error = "unsupported scene version";
			return false;
		}
		if (!root.Find("entities") || !root.Find("entities")->TryGetObject())
		{
			error = "scene root requires an entities block";
			return false;
		}

		written += "scene ";
		if (!AppendEscapedString(written, *name, error))
			return false;
		written += "\r\nversion: ";
		written += std::to_string(*version);
		written += "\r\n\r\n";

		if (!AppendRootBlock("assets", root.Find("assets"), written, error))
			return false;
		if (root.Find("assets"))
			written += "\r\n";
		if (!AppendRootBlock("systems", root.Find("systems"), written, error))
			return false;
		if (root.Find("systems"))
			written += "\r\n";
		if (!AppendRootBlock("entities", root.Find("entities"), written, error))
			return false;

		output = std::move(written);
		return true;
	}
}
