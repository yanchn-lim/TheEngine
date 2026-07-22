#version 460 core

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTint;

void main()
{
    FragColor = texture(uTexture, vTexCoord) * vec4(vColor, 1.0) * uTint;
}
