#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants {
    vec4 position; // x, y, width, height
} push;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    // Calculate final position (convert from pixel coordinates to clip space)
    vec2 screenPos = push.position.xy + (inPosition * push.position.zw);

    // Convert to clip space (adjust based on your screen dimensions)
    vec2 clipPos = (screenPos / vec2(1280.0, 720.0)) * 2.0 - 1.0;

    // Flip Y-axis for Vulkan's coordinate system
    clipPos.y = -clipPos.y;

    gl_Position = vec4(clipPos, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}