#pragma once
#include <glm/glm.hpp>
class Camera {
    glm::vec3 m_Position, m_Front, m_Up;
    float m_Yaw, m_Pitch;
public:
    Camera();
    glm::mat4 GetViewMatrix() const;
    void ProcessMouseMovement(float xoffset, float yoffset);
private:
    void UpdateVectors();
}; 