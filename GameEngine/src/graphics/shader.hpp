#pragma once

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace Graphics
{
	class Shader
	{
	private:
		uint32_t _id = 0;

		uint32_t _compileshader(uint32_t type, const std::string& source);

	public:
		bool Create(const std::string&, const std::string&);
		void Bind() const;
		void Destroy();

		//setters
		void SetInt(const char*, const int) const;
		void SetMat4(const char*, const glm::mat4&) const;

		//accessors
		uint32_t GetId() const;
		bool IsValid() const;

		//ctors
		Shader() = default;
		~Shader();
		//prevent copying
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;
		Shader(Shader&&) noexcept;
		Shader& operator=(Shader&&) noexcept;
	};
}
