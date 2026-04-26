#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    Camera();

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    glm::vec3 GetEyePosition() const;

    void SetTarget(const glm::vec3& target);
    void SetDistance(float distance);
    void FitSphere(const glm::vec3& center, float radius);

    void ProcessMouseMove(float xpos, float ypos, bool leftButtonDown);
    void ProcessScroll(float yoffset);

    void SetViewportSize(int width, int height);

private:
    void ClampPitch();

    glm::vec3 m_Target{0.0f};
    float m_Distance = 10.0f;
    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
    float m_FovDegrees = 45.0f;
    float m_Near = 0.1f;
    float m_Far = 500.0f;
    int m_ViewportWidth = 1;
    int m_ViewportHeight = 1;

    bool m_FirstMouse = true;
    float m_LastX = 0.0f;
    float m_LastY = 0.0f;
};
