#include "Application.h"
Application::Application() : m_Window("Protein Visualizer", 1280, 720) {}
void Application::Run() {
    while (!m_Window.ShouldClose()) {
        ProcessInput();
        // Render
        m_Window.Update();
    }
}
void Application::ProcessInput() {
    if (glfwGetKey(m_Window.GetNativeWindow(), GLFW_KEY_W) == GLFW_PRESS)
        m_Camera.ProcessMouseMovement(0.1f, 0.0f);
    // ... other controls
} 