#include "Camera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera() = default;

glm::vec3 OrbitPosition(const glm::vec3& target, float distance, float yawDeg, float pitchDeg) {
    const float yaw = glm::radians(yawDeg);
    const float pitch = glm::radians(pitchDeg);
    const float x = distance * std::cos(pitch) * std::cos(yaw);
    const float y = distance * std::sin(pitch);
    const float z = distance * std::cos(pitch) * std::sin(yaw);
    return target + glm::vec3(x, y, z);
}

glm::vec3 Camera::GetEyePosition() const {
    return OrbitPosition(m_Target, m_Distance, m_Yaw, m_Pitch);
}

glm::mat4 Camera::GetViewMatrix() const {
    const glm::vec3 eye = GetEyePosition();
    return glm::lookAt(eye, m_Target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_FovDegrees), aspect, m_Near, m_Far);
}

void Camera::SetTarget(const glm::vec3& target) {
    m_Target = target;
}

void Camera::SetDistance(float distance) {
    m_Distance = std::max(0.5f, distance);
}

void Camera::FitSphere(const glm::vec3& center, float radius) {
    m_Target = center;
    if (radius <= 1e-4f) {
        m_Distance = 10.0f;
        return;
    }
    const float aspect = static_cast<float>(std::max(1, m_ViewportWidth)) /
                         static_cast<float>(std::max(1, m_ViewportHeight));
    const float fov = glm::radians(m_FovDegrees);
    const float fit = radius / std::sin(fov * 0.5f);
    m_Distance = std::max(2.0f, fit * 1.35f);
}

void Camera::ProcessMouseMove(float xpos, float ypos, bool leftButtonDown) {
    if (!leftButtonDown) {
        m_FirstMouse = true;
        return;
    }
    if (m_FirstMouse) {
        m_LastX = xpos;
        m_LastY = ypos;
        m_FirstMouse = false;
    }
    float xoffset = xpos - m_LastX;
    float yoffset = m_LastY - ypos;
    m_LastX = xpos;
    m_LastY = ypos;
    const float sensitivity = 0.2f;
    m_Yaw += xoffset * sensitivity;
    m_Pitch += yoffset * sensitivity;
    ClampPitch();
}

void Camera::ProcessScroll(float yoffset) {
    if (yoffset == 0.0f) {
        return;
    }
    m_Distance *= (yoffset > 0.0f) ? 0.9f : 1.1f;
    m_Distance = std::clamp(m_Distance, 0.5f, 500.0f);
}

void Camera::SetViewportSize(int width, int height) {
    m_ViewportWidth = std::max(1, width);
    m_ViewportHeight = std::max(1, height);
}

void Camera::ClampPitch() {
    m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);
}
