#version 460 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

layout(location = 0) out vec3 vNearPoint;
layout(location = 1) out vec3 vFarPoint;

vec3 Unproject(vec2 position, float depth)
{
    // reconstruct a world-space point from an OpenGL clip-space depth
    vec4 point =
        inverse(uProjection * uView) * vec4(position, depth, 1.0);
    return point.xyz / point.w;
}

void main()
{
    // provide one world-space ray for each covered viewport pixel
    vNearPoint = Unproject(aPosition.xy, -1.0);
    vFarPoint = Unproject(aPosition.xy, 1.0);
    gl_Position = vec4(aPosition, 1.0);
}
