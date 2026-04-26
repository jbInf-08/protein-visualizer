#pragma once

struct GLFWwindow;

class Window {
public:
    Window(const char* title, int width, int height);
    ~Window();

    void Update();
    bool ShouldClose() const;
    bool IsUsable() const { return m_Window != nullptr; }

    GLFWwindow* GetNativeWindow() const { return m_Window; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    void NotifyFramebufferResize(int width, int height);

private:
    bool m_GlfwInitialized = false;
    GLFWwindow* m_Window = nullptr;
    const char* m_Title = nullptr;
    int m_Width = 0;
    int m_Height = 0;
};
