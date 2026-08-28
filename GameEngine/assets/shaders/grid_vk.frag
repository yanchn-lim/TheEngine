#version 460

layout(location = 0) in vec3 vNearPoint;
layout(location = 1) in vec3 vFarPoint;
layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform DrawConstants
{
    mat4 mvp;
    vec4 tint;
    vec4 uvRect;
} drawData;

float GridLine(vec2 worldPosition, float spacing)
{
    // use screen derivatives to keep grid lines close to one pixel wide
    vec2 coordinate = worldPosition / spacing;
    vec2 derivative = max(fwidth(coordinate), vec2(0.000001));
    vec2 distanceToLine =
        abs(fract(coordinate - 0.5) - 0.5) / derivative;
    return 1.0 - min(min(distanceToLine.x, distanceToLine.y), 1.0);
}

void main()
{
    // intersect the reconstructed camera ray with the world y = 0 plane
    float denominator = vFarPoint.y - vNearPoint.y;
    if (abs(denominator) < 0.00001)
        discard;

    float t = -vNearPoint.y / denominator;
    if (t < 0.0 || t > 1.0)
        discard;

    vec3 worldPosition = mix(vNearPoint, vFarPoint, t);
    float minor = GridLine(worldPosition.xz, 0.1);
    float major = GridLine(worldPosition.xz, 1.0);
    float line = max(minor * 0.35, major);

    // reduce aliasing and visual noise near the horizon and far distance
    float distanceFade = 1.0 -
        smoothstep(10.0, 100.0, distance(vNearPoint, worldPosition));
    float angleFade = smoothstep(
        0.02, 0.15, abs(normalize(vFarPoint - vNearPoint).y));
    float alpha = line * distanceFade * angleFade;
    if (alpha < 0.01)
        discard;

    vec3 color = mix(vec3(0.12), vec3(0.32), major);
    float xAxisWidth = max(fwidth(worldPosition.z) * 1.5, 0.001);
    float zAxisWidth = max(fwidth(worldPosition.x) * 1.5, 0.001);
    if (abs(worldPosition.z) < xAxisWidth)
        color = vec3(0.90, 0.22, 0.20);
    if (abs(worldPosition.x) < zAxisWidth)
        color = vec3(0.20, 0.42, 0.90);

    // Vulkan normalized depth already uses the depth buffer range 0..1
    vec4 clip = drawData.mvp * vec4(worldPosition, 1.0);
    gl_FragDepth = clip.z / clip.w;
    FragColor = vec4(color, alpha);
}
