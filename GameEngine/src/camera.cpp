#include "camera.hpp"

mat4 Camera2D::GetViewProjection(int2 viewportSize) const
{
    // half extents in world units (pixels), adjusted by zoom
    float halfW = (viewportSize.x * 0.5f) / zoom;
    float halfH = (viewportSize.y * 0.5f) / zoom;

    // projection: maps world space to NDC
    mat4 projection = glm::ortho(
        -halfW, halfW,   // left, right
        -halfH, halfH,   // bottom, top
        -1.f, 1.f      // near, far
    );

    // view: inverse of camera transform
    // translate by negative position to move the world opposite to the camera
    mat4 view = glm::translate(mat4(1.f), float3(-position, 0.f));

    return projection * view;
}

void Camera2D::ProcessPan(float2 delta)
{
    // delta is in screen pixels - scale by zoom so panning feels
    // consistent regardless of zoom level
    position.x += delta.x / zoom;
    position.y -= delta.y / zoom;
}

void Camera2D::ProcessZoom(float scrollDelta)
{
    constexpr float zoomSpeed = 0.1f;
    zoom *= 1.f + scrollDelta * zoomSpeed;
    zoom = glm::clamp(zoom, zoomMin, zoomMax);
}