#pragma once

struct Camera2D
{
	float2 position{ 0.f,0.f };
	float zoom = 1.f;
	float zoomMin = 0.01f;
	float zoomMax = 10.f;

    // returns combined view * projection matrix
    // viewport size needed to build ortho bounds
    mat4 GetViewProjection(int2 viewportSize) const;

    void ProcessPan(float2 delta);       // delta in screen pixels
    void ProcessZoom(float scrollDelta); // scrollDelta from GLFW scroll callback
};