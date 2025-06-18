#ifndef WOMPMATH_H
#define WOMPMATH_H

#include <glm/glm.hpp>

struct WP_Rect {
    float x, y, width, height;

    WP_Rect() = default;
    WP_Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
    WP_Rect(glm::vec2 position, glm::vec2 size) : x(position.x), y(position.y), width(size.x), height(size.y) {}

    glm::vec2 position() const { return {x, y}; }
    glm::vec2 size() const { return {width, height}; }

    glm::vec4 toVec4() const { return {x, y, width, height}; }

    static WP_Rect fromVec4(const glm::vec4& v) {
        return WP_Rect{v.x, v.y, v.z, v.w};
    }
};

#endif //WOMPMATH_H
