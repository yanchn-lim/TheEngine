#version 460 core

in vec3 vColor;
in vec2 vTexCoord;
in vec4 vPosition;

uniform sampler2D uTexture;

out vec4 FragColor;

vec3 hash(vec3 p)
{
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
              dot(p, vec3(269.5, 183.3, 246.1)),
              dot(p, vec3(113.5, 271.9, 124.6)));

    return fract(sin(p) * 43758.5453123);
}

// Robust integer hash (based on a common PCG-style mix)
uint hashUint(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

vec3 hashID(int id)
{
    uint h = hashUint(uint(id));
    
    // derive 3 different values by re-hashing with offsets
    uint hr = hashUint(h + 0u);
    uint hg = hashUint(h + 1u);
    uint hb = hashUint(h + 2u);

    return vec3(
        float(hr & 0xFFFFFFu) / float(0xFFFFFF),
        float(hg & 0xFFFFFFu) / float(0xFFFFFF),
        float(hb & 0xFFFFFFu) / float(0xFFFFFF)
    );
}

void main()
{
	//FragColor = vec4(vTexCoord,0.0,1.0);
	//FragColor = vPosition;
	//FragColor = vec4(hash(vPosition.xyz),1.0);
	FragColor = texture(uTexture, vTexCoord);
    //FragColor = vec4(hashID(gl_PrimitiveID), 1.0);
}
