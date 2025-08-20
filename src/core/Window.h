#pragma once
#include <GLFW/glfw3.h>
class Window {
    GLFWwindow* m_Window;
    const char* m_Title;
    int m_Width, m_Height;
public:
    Window(const char* title, int width, int height);
    ~Window();
    void Update();
    bool ShouldClose() const;
    GLFWwindow* GetNativeWindow() const { return m_Window; }
}; 