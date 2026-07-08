#version 460 core

in vec3 vColor;
in vec2 vTexCoord;
in vec4 vPosition;

uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
	//FragColor = vec4(vTexCoord,0.0,1.0);
	FragColor = vPosition;
}
