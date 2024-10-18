layout (binding = 0) uniform ViewUniformBuffer {
    mat4 ModelTrans;
    mat4 ViewTrans;
    mat4 ProjTrans;
    vec3 DebugColor;
    float DeltaTime;
};
