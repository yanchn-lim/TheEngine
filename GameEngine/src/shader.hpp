#pragma once

class Shader
{
public:
	Shader() = default;
	Shader(const std::string& vertPath, const std::string& fragPath);

	void Bind() const;
	void Unbind() const;

	void SetInt(const char* name, int value)              const;
	void SetFloat(const char* name, float value)            const;
	void SetMat4(const char* name, const mat4& value)      const;
	void SetFloat3(const char* name, const float3& value)   const;

	bool IsValid() const { return _program != 0; }

private:
	uint _program = 0;
	int GetLocation(const char* name) const;

	static std::string ReadFile(const std::string& path);
	static uint CompileStage(const char* src, uint type);
	static uint Link(uint vert, uint frag);
};


class ShaderLibrary
{
public:
	static ShaderLibrary& Get() { static ShaderLibrary instance; return instance; }
	ShaderLibrary(const ShaderLibrary&) = delete;
	ShaderLibrary& operator=(const ShaderLibrary&) = delete;

	bool Load(const std::string& name, const std::string& vertPath, const std::string& fragPath);
	Shader& Get(const std::string& name);
	bool Has(const std::string& name) const;

private:
	ShaderLibrary() = default;
	std::unordered_map<std::string, Shader> _shaders;
};