#pragma once

struct GLFWwindow;

#include "Window.h"
#include "../model/Protein.h"
#include "../renderer/Camera.h"
#include "../renderer/Renderer.h"

#include <array>
#include <optional>
#include <string>

class Application {
public:
    Application();
    void Run();

private:
    void RenderPanel();
    void UpdateCameraFromInput();
    void UpdateHoverPick();
    void TryLoadPdb(const std::string& path);
    void ResetCameraToMolecule();
    void OnFramebufferResize(int width, int height);

    static void GlfwFramebuffer(GLFWwindow* window, int width, int height);
    static void GlfwDrop(GLFWwindow* window, int count, const char** paths);

    Window m_Window;
    Camera m_Camera;
    Renderer m_Renderer;
    std::optional<Protein> m_Protein;
    std::array<char, 1024> m_PdbPathBuffer{};
    std::array<char, 1024> m_ScreenshotPathBuffer{};
    std::string m_LastLoadMessage;
    float m_ClearColorRgb[3]{0.07f, 0.07f, 0.09f};
    std::optional<int> m_HoverAtomIndex;
    bool m_RequestScreenshot = false;
    bool m_StrictDsspMode = true;
    bool m_LastLoadedStrictDsspMode = true;
    bool m_HasSuccessfulLoad = false;
};
