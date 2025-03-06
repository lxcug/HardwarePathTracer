#pragma shader_stage(closesthit)

#include "../../../src/host_device_shared/ViewUniformBuffer.h"
#include "RayTracingCommon.hlsl"
#include "Utils.hlsl"


[shader("closesthit")]
void main(inout RayPayload payload, in HitAttribute attrib)
{
    payload.seed = tea(attrib.bary.x * 12345u + attrib.bary.y * 54321u, view_uniform_buffer.FrameNum);
    payload.instance_id = InstanceID();
    payload.primitive_id = PrimitiveIndex();
    payload.hit_t = RayTCurrent();
    payload.bary = attrib.bary;
    payload.obj2world = ObjectToWorld4x3();
    payload.world2obj = WorldToObject4x3();

//     float3 bary_centrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
//
//     Vertex3 Vertices = GetVertices(InstanceID(), PrimitiveIndex());
//     Vertex v0 = Vertices.V0, v1 = Vertices.V1, v2 = Vertices.V2;
//
//     float2 hit_uv = v0.TexCoord * bary_centrics.x + v1.TexCoord * bary_centrics.y + v2.TexCoord * bary_centrics.z;
//     float3 hit_pos_obj_space = v0.Pos * bary_centrics.x + v1.Pos * bary_centrics.y + v2.Pos * bary_centrics.z;
//     float3 hit_normal_obj_space = v0.Normal * bary_centrics.x + v1.Normal * bary_centrics.y + v2.Normal * bary_centrics.z;
//     float3 hit_normal = normalize(mul((float3x3)ObjectToWorld3x4(), hit_normal_obj_space));
//     float3 hit_pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
//     // hit_pos = mul(float4(hit_pos_obj_space, 1.f), ObjectToWorld4x3());
//
//     InputMaterial input_mat = GetInputMaterial(InstanceID());
//     ShaderMaterial shader_mat = ResolveShaderMaterial(input_mat, hit_uv, hit_normal, v0.Tangent);
//
//     bool is_front_face = dot(payload.normal, -WorldRayDirection()) > 0.f;
//     payload.is_front_face = is_front_face;
//     payload.pos = hit_pos;
//     payload.base_color = shader_mat.base_color;
//     payload.normal = shader_mat.normal;
//     payload.emission = shader_mat.emission;
//     payload.opacity = shader_mat.opacity;
//     payload.roughness = shader_mat.roughness;
//     payload.metallic = shader_mat.metallic;
//     payload.ior = shader_mat.ior;
//     payload.transmittance = shader_mat.transmission;
//     payload.hit_t = RayTCurrent();
}
