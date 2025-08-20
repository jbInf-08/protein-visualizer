#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera() : m_Position(0,0,5), m_Front(0,0,-1), m_Up(0,1,0), m_Yaw(-90), m_Pitch(0) { UpdateVectors(); }

glm::mat4 Camera::GetViewMatrix() const { return glm::lookAt(m_Position, m_Position + m_Front, m_Up); }

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    m_Yaw += xoffset; m_Pitch += yoffset; UpdateVectors();
}

void Camera::UpdateVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);
} 