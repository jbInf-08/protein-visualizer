#include "Window.h"

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

Window::Window(const char* title, int width, int height)
    : m_Title(title), m_Width(width), m_Height(height) {
    if (!glfwInit()) {
        return;
    }
    m_GlfwInitialized = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        m_GlfwInitialized = false;
        return;
    }
    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
        glfwTerminate();
        m_GlfwInitialized = false;
        return;
    }

    glfwSetWindowUserPointer(m_Window, this);
}

Window::~Window() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    if (m_GlfwInitialized) {
        glfwTerminate();
        m_GlfwInitialized = false;
    }
}

void Window::Update() {
    glfwPollEvents();
    glfwSwapBuffers(m_Window);
}

bool Window::ShouldClose() const {
    return m_Window ? glfwWindowShouldClose(m_Window) != 0 : true;
}

void Window::NotifyFramebufferResize(int width, int height) {
    m_Width = width;
    m_Height = height;
}
