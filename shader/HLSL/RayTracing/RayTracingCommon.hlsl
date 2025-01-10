#define PI 3.1415926
#define HIT_COLOR float3(1., 1., 1.)
#define MISS_COLOR float3(0., 0., 0.)


RaytracingAccelerationStructure TLAS : register(t1);
RWTexture2D<float4> OutImage : register(u2);

struct RayPayload
{
    float3 pos;
    float3 normal;
    float3 color;
    bool is_hit;
};

struct HitAttribute
{
    float3 normal;
};


// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg(inout uint prev)
{
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = (LCG_A * prev + LCG_C);
  return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint seed)
{
  return (float(lcg(seed)) / float(0x01000000));
}

