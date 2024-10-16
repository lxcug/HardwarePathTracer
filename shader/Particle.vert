#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;  // not used

layout (location = 0) out vec3 fragColor;

layout (binding = 0) uniform ViewUniformBuffer {
    mat4 ModelTrans;
    mat4 ViewTrans;
    mat4 ProjTrans;
    vec3 DebugColor;
    float DeltaTime;
};

void main() {
    gl_Position = ProjTrans * ViewTrans * ModelTrans * vec4(aPos, 1.f);
    gl_PointSize = 1.0;
    fragColor = aColor;
}
