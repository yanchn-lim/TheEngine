#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
out vec2 vTexCoord;
out vec4 vPosition;

void main()
{
	vColor = aColor;
	vTexCoord = aTexCoord;
	gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
	vPosition = gl_Position;
}
