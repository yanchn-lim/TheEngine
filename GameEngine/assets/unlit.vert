#version 460 core

// Per-vertex
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aCol;
layout(location = 2) in vec2 aUV;

// Per-instance (divisor = 1, set in InstanceBatch::Init)
layout(location = 3) in vec3  iPosition;
layout(location = 4) in float iRotation;
layout(location = 5) in vec3  iScale;
layout(location = 6) in vec4  iTint;

uniform mat4 uViewProjection;

out vec2 vUV;
out vec4 vTint;

void main()
{
    float cosR = cos(iRotation);
    float sinR = sin(iRotation);

    // Reconstruct model matrix from instance data.
    // Column-major (GLM convention): each vec4 is one column.
    mat4 model = mat4(
        vec4( cosR * iScale.x,  sinR * iScale.x, 0.0, 0.0),
        vec4(-sinR * iScale.y,  cosR * iScale.y, 0.0, 0.0),
        vec4( 0.0,              0.0,              1.0, 0.0),
        vec4( iPosition.x,      iPosition.y,      iPosition.z, 1.0)
    );

    gl_Position = uViewProjection * model * vec4(aPos, 1.0);
    vUV   = aUV;
    vTint = iTint * vec4(aCol, 1.0);
}