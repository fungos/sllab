#pragma once

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox(glm::vec3 point)
        : min(point)
        , max(point)
    {
    }

    BoundingBox()
        : min({kMaxFloat, kMaxFloat, kMaxFloat})
        , max({kMinFloat, kMinFloat, kMinFloat})
    {
    }

    BoundingBox(eNotInitialized) {
    }

    void grow(glm::vec3 point) {
        min.x = (point.x < min.x) ? point.x : min.x;
        min.y = (point.y < min.y) ? point.y : min.y;
        min.z = (point.z < min.z) ? point.z : min.z;
        max.x = (point.x > max.x) ? point.x : max.x;
        max.y = (point.y > max.y) ? point.y : max.y;
        max.z = (point.z > max.z) ? point.z : max.z;
    }

    template <class T>
    void grow(T &&other) {
        grow(other.min);
        grow(other.max);
    }
};
