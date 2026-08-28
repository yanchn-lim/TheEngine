#version 460

layout(location = 0) in vec3 aPosition;

layout(push_constant) uniform DrawConstants
{
    mat4 mvp;
    vec4 tint;
    vec4 uvRect;
} drawData;

layout(location = 0) out vec3 vNearPoint;
layout(location = 1) out vec3 vFarPoint;

vec3 Unproject(vec2 position, float depth)
{
    // reconstruct a world-space point from a Vulkan clip-space depth
    vec4 point = inverse(drawData.mvp) * vec4(position, depth, 1.0);
    return point.xyz / point.w;
}

void main()
{
    // provide one world-space ray for each covered viewport pixel
    vNearPoint = Unproject(aPosition.xy, 0.0);
    vFarPoint = Unproject(aPosition.xy, 1.0);
    gl_Position = vec4(aPosition, 1.0);
}
