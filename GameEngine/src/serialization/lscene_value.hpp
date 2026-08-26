#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Serialization
{
	struct SourceLocation
	{
		size_t line = 1;
		size_t column = 1;
	};

	class LSceneValue
	{
	public:
		using Array = std::vector<LSceneValue>;
		using Object = std::map<std::string, LSceneValue>;

		static LSceneValue Boolean(bool value, SourceLocation location)
		{
			return LSceneValue(value, location);
		}

		static LSceneValue Integer(int64_t value, SourceLocation location)
		{
			return LSceneValue(value, location);
		}

		static LSceneValue Float(double value, SourceLocation location)
		{
			return LSceneValue(value, location);
		}

		static LSceneValue String(std::string value, SourceLocation location)
		{
			return LSceneValue(std::move(value), location);
		}

		static LSceneValue ArrayValue(Array value, SourceLocation location)
		{
			return LSceneValue(std::move(value), location);
		}

		static LSceneValue ObjectValue(Object value = {}, SourceLocation location = {})
		{
			return LSceneValue(std::move(value), location);
		}

		SourceLocation GetLocation() const noexcept
		{
			return _location;
		}

		const LSceneValue* Find(std::string_view key) const
		{
			const Object* object = TryGetObject();
			if (!object)
				return nullptr;

			const auto found = object->find(std::string(key));
			return found == object->end() ? nullptr : &found->second;
		}

		const Array* TryGetArray() const noexcept { return std::get_if<Array>(&_data); }
		const Object* TryGetObject() const noexcept { return std::get_if<Object>(&_data); }
		const std::string* TryGetString() const noexcept { return std::get_if<std::string>(&_data); }
		const int64_t* TryGetInteger() const noexcept { return std::get_if<int64_t>(&_data); }
		const double* TryGetFloat() const noexcept { return std::get_if<double>(&_data); }
		const bool* TryGetBoolean() const noexcept { return std::get_if<bool>(&_data); }
		Object* TryGetObject() noexcept { return std::get_if<Object>(&_data); }

	private:
		using Data = std::variant<std::monostate, bool, int64_t, double, std::string, Array, Object>;

		template<typename Value>
		explicit LSceneValue(Value value, SourceLocation location)
			: _data(std::move(value)), _location(location)
		{
		}

		Data _data;
		SourceLocation _location;
	};
}
