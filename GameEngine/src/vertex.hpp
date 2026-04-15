#pragma once

// One point on a mesh. Position is in local (object) space; colour is RGB in [0,1].
struct Vertex
{
    float3 pos; // vert pos
    float3 col{1.f,1.f,1.f}; // vert col

    float2 texcoords;  // tex coords
};
