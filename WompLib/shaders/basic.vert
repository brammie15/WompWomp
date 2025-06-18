#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform screenUniforms {
    vec2 screenSize;
} ubo;

layout(push_constant) uniform PushConstants {
    vec4 srcRect;   // xy = UV offset, zw = UV size
    vec4 dstRect;   // xy = screen position (pixels), zw = size (pixels)
    float rotation;
    vec4 color;
} push;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    vec2 localPos = inPosition.xy * push.dstRect.zw;

    float c = cos(push.rotation);
    float s = sin(push.rotation);
    mat2 rot = mat2(c, -s, s, c);
    vec2 rotatedPos = rot * localPos;

    vec2 screenPos = push.dstRect.xy + rotatedPos;

    // Normalize to NDC
    vec2 clipPos = (screenPos / ubo.screenSize) * 2.0 - 1.0;
    clipPos.y = -clipPos.y; // Flip Y for Vulkan

    gl_Position = vec4(clipPos, 0.0, 1.0);

    fragTexCoord = push.srcRect.xy + inTexCoord * push.srcRect.zw;
}
