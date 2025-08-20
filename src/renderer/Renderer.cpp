#include "Renderer.h"
Renderer::Renderer() : m_CurrentMode(VisualizationMode::BallAndStick) {}
void Renderer::SetVisualizationMode(VisualizationMode mode) { m_CurrentMode = mode; }
void Renderer::RenderProtein(const Protein& protein) {
    switch (m_CurrentMode) {
        case VisualizationMode::BallAndStick: RenderBallAndStick(protein); break;
        case VisualizationMode::SpaceFilling: RenderSpaceFilling(protein); break;
        // ... other modes
    }
}
void Renderer::RenderBallAndStick(const Protein& protein) { /* OpenGL draw calls */ }
void Renderer::RenderSpaceFilling(const Protein& protein) { /* OpenGL draw calls */ } 