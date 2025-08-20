#pragma once
#include <glm/glm.hpp>
#include <string>
struct Atom {
    glm::vec3 position;
    std::string element;
    float radius;
    int chainId;
}; 