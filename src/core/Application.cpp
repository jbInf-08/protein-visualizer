#include "Application.h"

#include "Screenshot.h"

#include "../utils/Picking.h"
#include "../utils/PDBParser.h"
#include "../utils/StructureLoader.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace {
Application* g_App = nullptr;
}

Application::Application()
    : m_Window("Protein Visualizer", 1280, 720) {
    const char* def = "resources/1CRN.pdb";
    std::strncpy(m_PdbPathBuffer.data(), def, m_PdbPathBuffer.size() - 1);
    m_PdbPathBuffer[m_PdbPathBuffer.size() - 1] = '\0';
    const char* shot = "screenshot.png";
    std::strncpy(m_ScreenshotPathBuffer.data(), shot, m_ScreenshotPathBuffer.size() - 1);
    m_ScreenshotPathBuffer[m_ScreenshotPathBuffer.size() - 1] = '\0';
}

void Application::TryLoadPdb(const std::string& path) {
    m_LastLoadMessage.clear();
    if (!path.empty()) {
        std::strncpy(m_PdbPathBuffer.data(), path.c_str(), m_PdbPathBuffer.size() - 1);
        m_PdbPathBuffer[m_PdbPathBuffer.size() - 1] = '\0';
    }
    try {
        if (path.empty()) {
            m_LastLoadMessage = "Path is empty.";
            return;
        }
        PDBParser::SetStrictDsspMode(m_StrictDsspMode);
        const std::filesystem::path p(path);
        if (!std::filesystem::exists(p)) {
            m_LastLoadMessage = "File not found: " + path;
            return;
        }
        Protein protein = StructureLoader::Load(path);
        m_Protein = std::move(protein);
        m_HasSuccessfulLoad = true;
        m_LastLoadedStrictDsspMode = m_StrictDsspMode;
        m_Renderer.SetProtein(&*m_Protein);
        const glm::vec3 center = m_Protein->GetCenter();
        const float radius = m_Protein->GetBoundingRadius();
        m_Camera.SetTarget(center);
        m_Camera.FitSphere(center, std::max(radius, 0.5f));
        m_LastLoadMessage = "Loaded " + std::to_string(m_Protein->GetAtoms().size()) + " atoms, " +
                            std::to_string(m_Protein->GetBonds().size()) + " bonds, " +
                            std::to_string(m_Protein->GetCaTubeSegments().size()) + " Cα segments from " + path;
    } catch (const std::exception& e) {
        m_HasSuccessfulLoad = false;
        m_Protein.reset();
        m_Renderer.SetProtein(nullptr);
        m_LastLoadMessage = std::string("Load failed: ") + e.what();
    }
}

void Application::ResetCameraToMolecule() {
    if (!m_Protein || !m_Protein->HasAtoms()) {
        return;
    }
    const glm::vec3 center = m_Protein->GetCenter();
    const float radius = m_Protein->GetBoundingRadius();
    m_Camera.SetTarget(center);
    m_Camera.FitSphere(center, std::max(radius, 0.5f));
}

void Application::OnFramebufferResize(int width, int height) {
    m_Window.NotifyFramebufferResize(width, height);
    glViewport(0, 0, width, height);
    m_Camera.SetViewportSize(width, height);
}

void Application::GlfwFramebuffer(GLFWwindow* window, int width, int height) {
    (void)window;
    // ImGui 1.91+ refreshes io.DisplaySize each ImGui_ImplGlfw_NewFrame(); no exported framebuffer callback.
    if (g_App) {
        g_App->OnFramebufferResize(width, height);
    }
}

void Application::GlfwDrop(GLFWwindow*, int count, const char** paths) {
    if (!g_App || count <= 0 || !paths || !paths[0]) {
        return;
    }
    g_App->TryLoadPdb(std::string(paths[0]));
}

void Application::UpdateHoverPick() {
    ImGuiIO& io = ImGui::GetIO();
    if (!m_Protein || !m_Protein->HasAtoms() || io.WantCaptureMouse) {
        m_HoverAtomIndex.reset();
        return;
    }
    GLFWwindow* win = m_Window.GetNativeWindow();
    if (!win) {
        m_HoverAtomIndex.reset();
        return;
    }
    int fbw = 0;
    int fbh = 0;
    glfwGetFramebufferSize(win, &fbw, &fbh);
    int ww = 0;
    int wh = 0;
    glfwGetWindowSize(win, &ww, &wh);
    if (fbw <= 0 || fbh <= 0 || ww <= 0 || wh <= 0) {
        m_HoverAtomIndex.reset();
        return;
    }
    double mx = 0.0;
    double my = 0.0;
    glfwGetCursorPos(win, &mx, &my);
    const float sx = static_cast<float>(fbw) / static_cast<float>(ww);
    const float sy = static_cast<float>(fbh) / static_cast<float>(wh);
    const float px = static_cast<float>(mx) * sx;
    const float py = static_cast<float>(my) * sy;

    const float aspect = static_cast<float>(fbw) / static_cast<float>(fbh);
    const glm::mat4 view = m_Camera.GetViewMatrix();
    const glm::mat4 projection = m_Camera.GetProjectionMatrix(aspect);
    m_HoverAtomIndex = Picking::PickAtomIndexScreen(*m_Protein, px, py, fbw, fbh, view, projection,
                                                    m_Renderer.GetVisualizationMode());
}

void Application::UpdateCameraFromInput() {
    ImGuiIO& io = ImGui::GetIO();
    GLFWwindow* win = m_Window.GetNativeWindow();
    if (!win) {
        return;
    }
    if (!io.WantCaptureMouse) {
        double mx = 0.0;
        double my = 0.0;
        glfwGetCursorPos(win, &mx, &my);
        const int left = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT);
        m_Camera.ProcessMouseMove(static_cast<float>(mx), static_cast<float>(my), left == GLFW_PRESS);
        m_Camera.ProcessScroll(io.MouseWheel);
    }
}

void Application::RenderPanel() {
    ImGui::Begin("Molecule");
    ImGui::InputText("Structure path", m_PdbPathBuffer.data(), m_PdbPathBuffer.size());
    if (ImGui::Button("Load")) {
        TryLoadPdb(std::string(m_PdbPathBuffer.data()));
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset view")) {
        ResetCameraToMolecule();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Tip: drag and drop .pdb or .cif / .mmcif onto the window.");
    ImGui::Separator();
    ImGui::ColorEdit3("Background", m_ClearColorRgb);
    ImGui::Separator();

    int modeIndex = static_cast<int>(m_Renderer.GetVisualizationMode());
    const char* modeNames[] = {"Ball and stick", "Space filling", "Ribbon (Cα, SS colors)", "Cartoon (helix / strand)"};
    if (ImGui::Combo("Visualization", &modeIndex, modeNames, IM_ARRAYSIZE(modeNames))) {
        m_Renderer.SetVisualizationMode(static_cast<VisualizationMode>(modeIndex));
    }
    if (ImGui::Checkbox("Strict DSSP mode (E/B/G/I/T labels)", &m_StrictDsspMode)) {
        PDBParser::SetStrictDsspMode(m_StrictDsspMode);
    }
    if (m_HasSuccessfulLoad) {
        ImGui::Text("Current loaded SS mode: %s", m_LastLoadedStrictDsspMode ? "Strict DSSP" : "Legacy DSSP-like");
        if (m_LastLoadedStrictDsspMode != m_StrictDsspMode) {
            ImGui::TextUnformatted("Mode changed. Reload to apply.");
        }
    } else {
        ImGui::Text("Current loaded SS mode: %s", m_StrictDsspMode ? "Strict DSSP (pending load)" : "Legacy DSSP-like (pending load)");
    }

    ImGui::Separator();
    ImGui::InputText("Screenshot PNG", m_ScreenshotPathBuffer.data(), m_ScreenshotPathBuffer.size());
    if (ImGui::Button("Save screenshot")) {
        m_RequestScreenshot = true;
    }

    if (m_Protein && m_Protein->HasAtoms() && m_HoverAtomIndex.has_value()) {
        const int i = *m_HoverAtomIndex;
        const auto& atoms = m_Protein->GetAtoms();
        if (i >= 0 && i < static_cast<int>(atoms.size())) {
            const Atom& a = atoms[static_cast<size_t>(i)];
            ImGui::Separator();
            ImGui::Text("Hover: %s %s %d (%s) serial %d", a.atomName.c_str(), a.residueName.c_str(), a.residueSeq,
                        a.element.c_str(), a.serial);
        }
    }

    if (!m_LastLoadMessage.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", m_LastLoadMessage.c_str());
    }
    ImGui::End();
}

void Application::Run() {
    if (!m_Window.IsUsable()) {
        std::cerr << "Failed to create GLFW window or OpenGL context.\n";
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(m_Window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    m_Camera.SetViewportSize(m_Window.GetWidth(), m_Window.GetHeight());
    g_App = this;
    glfwSetFramebufferSizeCallback(m_Window.GetNativeWindow(), &Application::GlfwFramebuffer);
    glfwSetDropCallback(m_Window.GetNativeWindow(), &Application::GlfwDrop);

    glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
    glEnable(GL_DEPTH_TEST);

    TryLoadPdb(std::string(m_PdbPathBuffer.data()));

    while (!m_Window.ShouldClose()) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        UpdateCameraFromInput();
        RenderPanel();

        ImGui::Render();

        glClearColor(m_ClearColorRgb[0], m_ClearColorRgb[1], m_ClearColorRgb[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLFWwindow* wnd = m_Window.GetNativeWindow();
        int fbw = 0;
        int fbh = 0;
        if (wnd) {
            glfwGetFramebufferSize(wnd, &fbw, &fbh);
        }
        if (fbw <= 0 || fbh <= 0) {
            fbw = std::max(1, m_Window.GetWidth());
            fbh = std::max(1, m_Window.GetHeight());
        }
        const float aspect = static_cast<float>(fbw) / static_cast<float>(fbh);
        const glm::mat4 view = m_Camera.GetViewMatrix();
        const glm::mat4 projection = m_Camera.GetProjectionMatrix(aspect);
        m_Renderer.Draw(fbw, fbh, view, projection);

        UpdateHoverPick();

        if (m_RequestScreenshot) {
            m_RequestScreenshot = false;
            const std::string shotPath(m_ScreenshotPathBuffer.data());
            if (SaveFramebufferPngRgb(fbw, fbh, shotPath)) {
                m_LastLoadMessage = "Saved screenshot: " + shotPath;
            } else {
                m_LastLoadMessage = "Screenshot failed: " + shotPath;
            }
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        m_Window.Update();
    }

    g_App = nullptr;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
