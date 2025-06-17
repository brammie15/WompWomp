#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants {
    vec4 position; // x, y, width, height
    float rotation;
} push;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    // Normalized position in [-0.5, 0.5] space
    vec2 centeredPos = inPosition.xy * push.position.zw;

    // Apply rotation
    float c = cos(push.rotation);
    float s = sin(push.rotation);
    mat2 rotationMatrix = mat2(c, -s, s, c);

    vec2 rotatedPos = rotationMatrix * centeredPos;

    // Offset by screen position
    vec2 screenPos = push.position.xy + rotatedPos;

    // Convert to clip space
    vec2 clipPos = (screenPos / vec2(1200.0, 1000.0)) * 2.0 - 1.0;
    clipPos.y = -clipPos.y;

    gl_Position = vec4(clipPos, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}
