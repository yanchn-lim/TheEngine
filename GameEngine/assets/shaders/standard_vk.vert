#version 460

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

layout(push_constant) uniform DrawConstants
{
    mat4 mvp;
    vec4 tint;
    vec4 uvRect;
} drawData;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vTexCoord;

void main()
{
    vColor = aColor;
    vTexCoord = mix(drawData.uvRect.xy, drawData.uvRect.zw, aTexCoord);
    gl_Position = drawData.mvp * vec4(aPosition, 1.0);
}
