#version 460

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D uTexture;

layout(push_constant) uniform DrawConstants
{
    mat4 mvp;
    vec4 tint;
    vec4 uvRect;
} drawData;

void main()
{
    FragColor = texture(uTexture, vTexCoord) * vec4(vColor, 1.0) * drawData.tint;
}
