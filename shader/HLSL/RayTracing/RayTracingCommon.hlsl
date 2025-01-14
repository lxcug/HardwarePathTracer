#define PI 3.1415926

struct Vertex {
    float3 Pos;
    uint PosPadding;
    float3 Normal;
    uint NormalPadding;
    float3 Color;
    uint ColorPadding;
    float2 TexCoord;
};

struct Vertex3 {
    Vertex V0, V1, V2;
};

struct ModelDesc {
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;
    uint64_t MaterialBufferAddress;
    uint64_t MaterialIndexBufferAddress;
    int TextureIndexOffset;
};

RaytracingAccelerationStructure TLAS : register(t1, space0);
RWTexture2D<float4> OutImage : register(u2, space0);
Texture2D<float4> InImage : register(t3, space0);

// TODO: Move to space1
StructuredBuffer<ModelDesc> ModelInfo : register(t4, space0);
Texture2D<float4> MaterialTextures[] : register(t5, space0);
SamplerState Samplers[] : register(s5, space0);

RWTexture2D<float4> GBuffer[] : register(u6, space0);


struct RayPayload
{
    float3 pos;
    float hit_t;

    float3 normal;
    uint instance_id;

    float3 albedo;
    uint triangle_index;

    bool is_hit;
    bool is_front_face;
};

struct HitAttribute
{
    float2 bary;
};

struct Material {
    float3 Albedo;
    int AlbedoTextureID;

    float3 Emissive;
    int EmissiveTextureID;

    float3 Transmittance;
    float Opacity;

    float Roughness;
    float Metallic;
    int RoughnessTextureID;
    int MetallicTextureID;
};

Vertex3 GetVertices(uint InstanceID, uint PrimitiveIndex) {
    uint64_t VertexBufferAddress = ModelInfo[InstanceID].VertexBufferAddress;
    uint64_t IndexBufferAddress = ModelInfo[InstanceID].IndexBufferAddress;
    uint3 Indices = {
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (PrimitiveIndex * 3 + 0)),
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (PrimitiveIndex * 3 + 1)),
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (PrimitiveIndex * 3 + 2))
    };

    Vertex3 Ret = {
        vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * Indices.x),
        vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * Indices.y),
        vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * Indices.z)
    };
    return Ret;
}

Material GetMaterial(uint InstanceID, uint PrimitiveIndex) {
    uint64_t MaterialBufferAddress = ModelInfo[InstanceID].MaterialBufferAddress;
    uint64_t MaterialIndexBufferAddress = ModelInfo[InstanceID].MaterialIndexBufferAddress;

    int material_index = vk::RawBufferLoad<int>(MaterialIndexBufferAddress + sizeof(int) * PrimitiveIndex);
    return vk::RawBufferLoad<Material>(MaterialBufferAddress + sizeof(Material) * material_index);
}


// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint tea(uint val0, uint val1)
{
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0;

  for(uint n = 0; n < 16; n++)
  {
    s0 += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
  }

  return v0;
}

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
