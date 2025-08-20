#pragma once
#include "../model/Protein.h"
enum class VisualizationMode { BallAndStick, SpaceFilling, Ribbon, Cartoon };
class Renderer {
    VisualizationMode m_CurrentMode;
public:
    Renderer();
    void SetVisualizationMode(VisualizationMode mode);
    void RenderProtein(const Protein& protein);
private:
    void RenderBallAndStick(const Protein& protein);
    void RenderSpaceFilling(const Protein& protein);
    // ... other modes
}; 