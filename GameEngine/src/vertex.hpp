#pragma once

// One point on a mesh. Position is in local (object) space; colour is RGB in [0,1].
struct Vertex
{
    float2 pos; // XY position in local space
    float3 col; // RGB colour
};
