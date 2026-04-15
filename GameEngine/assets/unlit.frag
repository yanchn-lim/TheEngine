#version 460 core

// Runs once per pixel. Outputs the colour interpolated across the triangle surface.
// "Unlit" means no lighting math - colour is written directly to the framebuffer.

in  vec3 vCol;       // colour blended across the triangle from the vertex shader
in  vec2 vUV;

uniform sampler2D uTexture;

out vec4 FragColor;  // final RGBA written to the framebuffer

void main()
{
    vec4 texColor = texture(uTexture, vUV);
    FragColor    = texColor;
}
