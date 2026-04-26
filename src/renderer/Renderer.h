#pragma once

#include "../model/Protein.h"

#include <glm/mat4x4.hpp>

enum class VisualizationMode { BallAndStick, SpaceFilling, Ribbon, Cartoon };

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void SetVisualizationMode(VisualizationMode mode);
    VisualizationMode GetVisualizationMode() const { return m_CurrentMode; }

    void SetProtein(const Protein* protein);

    void Draw(int viewportWidth, int viewportHeight, const glm::mat4& view, const glm::mat4& projection);

    static float RadiusScaleForMode(VisualizationMode mode, float vdwRadius);

private:
    void EnsureGpuReady();
    void RebuildInstanceBuffer();
    void RebuildCylinderInstanceBuffer();
    void DestroyGeometryBuffers();
    void CreateGeometryBuffers(int sphereStacks, int sphereSlices, int cylinderSlices);

    VisualizationMode m_CurrentMode = VisualizationMode::BallAndStick;
    const Protein* m_Protein = nullptr;
    bool m_InstancesDirty = true;
    bool m_CylinderInstancesDirty = true;

    unsigned int m_Vao = 0;
    unsigned int m_MeshVbo = 0;
    unsigned int m_Ebo = 0;
    unsigned int m_InstanceVbo = 0;
    unsigned int m_IndexCount = 0;
    unsigned int m_Program = 0;

    unsigned int m_CylVao = 0;
    unsigned int m_CylMeshVbo = 0;
    unsigned int m_CylEbo = 0;
    unsigned int m_CylInstanceVbo = 0;
    unsigned int m_CylIndexCount = 0;
    unsigned int m_CylProgram = 0;
    int m_CylinderInstanceCount = 0;

    int m_LodBracket = 0;
};
