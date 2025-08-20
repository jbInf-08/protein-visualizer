#pragma once
#include "Window.h"
#include "../renderer/Camera.h"
#include "../renderer/Renderer.h"
class Application {
    Window m_Window;
    Camera m_Camera;
    Renderer m_Renderer;
    void ProcessInput();
public:
    Application();
    void Run();
}; 