#pragma Compute UpdateParticles

#include "ViewUniformBuffer.hlsl"
#include "ParticleCommon.hlsl"

StructuredBuffer<Particle> ParticlesIn : register(t1);
RWStructuredBuffer<Particle> ParticlesOut : register(u2);

[numthreads(256, 1, 1)]
void UpdateParticles(
    uint3 GlobalThreadID : SV_DispatchThreadID
) {
    ParticlesOut[GlobalThreadID.x].Position = ParticlesIn[GlobalThreadID.x].Position +
        ParticlesIn[GlobalThreadID.x].Velocity * DeltaTime * .1f;
    ParticlesOut[GlobalThreadID.x].Velocity = ParticlesIn[GlobalThreadID.x].Velocity;
    ParticlesOut[GlobalThreadID.x].Color = ParticlesIn[GlobalThreadID.x].Color;
}
