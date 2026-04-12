#version 460 core

// Runs once per pixel. Outputs the colour interpolated across the triangle surface.
// "Unlit" means no lighting math - colour is written directly to the framebuffer.

in  vec3 vCol;       // colour blended across the triangle from the vertex shader
out vec4 FragColor;  // final RGBA written to the framebuffer

void main()
{
    FragColor = vec4(vCol, 1.0); // alpha forced to 1 (fully opaque)
}
