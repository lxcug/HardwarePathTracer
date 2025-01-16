float max(float3 value) {
    return max(value.x, max(value.y, value.z));
}