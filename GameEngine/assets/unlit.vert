#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aCol;

uniform mat4 uModel;
uniform mat4 uViewProjection;

out vec3 vCol;

void main()
{
    vCol        = aCol;
    gl_Position = uViewProjection * uModel * vec4(aPos, 0.0, 1.0);
}