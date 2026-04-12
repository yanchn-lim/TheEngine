#version 460 core

// Transforms each vertex from local space to clip space (what the GPU uses to place pixels on screen).
// Chain: local * uModel = world space, world * uViewProjection = clip space.

layout(location = 0) in vec2 aPos; // vertex position in local space
layout(location = 1) in vec3 aCol; // vertex colour, passed straight through

uniform mat4 uModel;          // local -> world  (position, rotation, scale of this object)
uniform mat4 uViewProjection; // world -> clip   (camera + projection)

out vec3 vCol; // interpolated colour for the fragment shader

void main()
{
    vCol        = aCol;
    gl_Position = uViewProjection * uModel * vec4(aPos, 0.0, 1.0); // promote 2D pos to 4D for matrix multiply
}
