#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    // Vulkan textures typically have origin at top-left, so flip Y if needed
    vec2 flippedUV = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);

    outColor = texture(texSampler, flippedUV);

    // Debug fallback: Uncomment for magenta if sampling fails visibly
    // outColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);

    // Optional debug: red overlay if texture alpha is zero
    // if (outColor.a == 0.0) {
    //     outColor = vec4(1.0, 0.0, 0.0, 0.5);
    // }
}
