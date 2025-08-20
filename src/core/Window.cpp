#include "Window.h"
Window::Window(const char* title, int width, int height)
    : m_Title(title), m_Width(width), m_Height(height) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    glfwMakeContextCurrent(m_Window);
}
Window::~Window() { glfwDestroyWindow(m_Window); glfwTerminate(); }
void Window::Update() { glfwPollEvents(); glfwSwapBuffers(m_Window); }
bool Window::ShouldClose() const { return glfwWindowShouldClose(m_Window); } 