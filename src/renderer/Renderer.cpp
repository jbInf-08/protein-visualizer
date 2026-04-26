#include "Renderer.h"

#include "../model/SecondaryStructure.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string ReadFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Failed to read file: " + path);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint CompileShader(GLenum type, const char* src) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<size_t>(std::max(1, len)), '\0');
        GLsizei out = 0;
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), &out, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile failed: ") + log);
    }
    return shader;
}

GLuint LinkProgram(GLuint vs, GLuint fs) {
    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<size_t>(std::max(1, len)), '\0');
        GLsizei out = 0;
        glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), &out, log.data());
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Program link failed: ") + log);
    }
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

GLuint CreateProgramFromFiles(const std::string& vertPath, const std::string& fragPath) {
    const std::string vsrc = ReadFile(vertPath);
    const std::string fsrc = ReadFile(fragPath);
    const GLuint vs = CompileShader(GL_VERTEX_SHADER, vsrc.c_str());
    const GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsrc.c_str());
    return LinkProgram(vs, fs);
}

void BuildUvUnitSphere(int stacks, int slices, std::vector<float>& interleaved, std::vector<unsigned int>& indices) {
    interleaved.clear();
    indices.clear();

    for (int i = 0; i <= stacks; ++i) {
        const float v = static_cast<float>(i) / static_cast<float>(stacks);
        const float phi = v * 3.14159265358979323846f;
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);
        for (int j = 0; j <= slices; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(slices);
            const float theta = u * 2.0f * 3.14159265358979323846f;
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);
            const float x = cosTheta * sinPhi;
            const float y = cosPhi;
            const float z = sinTheta * sinPhi;
            interleaved.push_back(x);
            interleaved.push_back(y);
            interleaved.push_back(z);
            interleaved.push_back(x);
            interleaved.push_back(y);
            interleaved.push_back(z);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (slices + 1);
        int k2 = k1 + slices + 1;
        for (int j = 0; j < slices; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(static_cast<unsigned int>(k1));
                indices.push_back(static_cast<unsigned int>(k2));
                indices.push_back(static_cast<unsigned int>(k1 + 1));
            }
            if (i != stacks - 1) {
                indices.push_back(static_cast<unsigned int>(k1 + 1));
                indices.push_back(static_cast<unsigned int>(k2));
                indices.push_back(static_cast<unsigned int>(k2 + 1));
            }
        }
    }
}

char ElementKey(const std::string& elem) {
    if (elem.empty()) {
        return 'X';
    }
    return static_cast<char>(std::toupper(static_cast<unsigned char>(elem[0])));
}

glm::vec3 SsColor(SSType ss) {
    switch (ss) {
        case SSType::Helix:
            return glm::vec3(0.32f, 0.55f, 1.0f);
        case SSType::Strand:
            return glm::vec3(1.0f, 0.82f, 0.22f);
        default:
            return glm::vec3(0.58f, 0.58f, 0.62f);
    }
}

glm::vec3 ElementColor(const std::string& elem) {
    switch (ElementKey(elem)) {
        case 'H':
            return glm::vec3(0.95f, 0.95f, 1.0f);
        case 'C':
            return glm::vec3(0.565f, 0.565f, 0.565f);
        case 'N':
            return glm::vec3(0.188f, 0.314f, 0.972f);
        case 'O':
            return glm::vec3(1.0f, 0.051f, 0.051f);
        case 'S':
            return glm::vec3(0.98f, 0.98f, 0.2f);
        case 'P':
            return glm::vec3(1.0f, 0.502f, 0.0f);
        default:
            return glm::vec3(0.9f, 0.4f, 0.9f);
    }
}

/// Open unit cylinder along +Y from y=0 to y=1, radius 1 in XZ; interleaved position(3)+normal(3).
void BuildUnitOpenCylinder(int slices, std::vector<float>& interleaved, std::vector<unsigned int>& indices) {
    interleaved.clear();
    indices.clear();
    const float twoPi = 2.0f * 3.14159265358979323846f;
    for (int i = 0; i <= slices; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(slices);
        const float a = t * twoPi;
        const float nx = std::cos(a);
        const float nz = std::sin(a);
        interleaved.push_back(nx);
        interleaved.push_back(0.0f);
        interleaved.push_back(nz);
        interleaved.push_back(nx);
        interleaved.push_back(0.0f);
        interleaved.push_back(nz);
        interleaved.push_back(nx);
        interleaved.push_back(1.0f);
        interleaved.push_back(nz);
        interleaved.push_back(nx);
        interleaved.push_back(0.0f);
        interleaved.push_back(nz);
    }
    for (int i = 0; i < slices; ++i) {
        const int b0 = i * 2;
        const int t0 = b0 + 1;
        const int b1 = b0 + 2;
        const int t1 = t0 + 2;
        indices.push_back(static_cast<unsigned int>(b0));
        indices.push_back(static_cast<unsigned int>(t0));
        indices.push_back(static_cast<unsigned int>(b1));
        indices.push_back(static_cast<unsigned int>(t1));
        indices.push_back(static_cast<unsigned int>(b1));
        indices.push_back(static_cast<unsigned int>(t0));
    }
}

void AppendMat4Color(std::vector<float>& out, const glm::mat4& m, const glm::vec4& color) {
    for (int c = 0; c < 4; ++c) {
        out.push_back(m[c].x);
        out.push_back(m[c].y);
        out.push_back(m[c].z);
        out.push_back(m[c].w);
    }
    out.push_back(color.x);
    out.push_back(color.y);
    out.push_back(color.z);
    out.push_back(color.w);
}

glm::vec3 CatmullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

struct TubeSegmentSample {
    glm::vec3 a{0.0f};
    glm::vec3 b{0.0f};
    SSType ss = SSType::Coil;
};

std::vector<TubeSegmentSample> BuildSmoothedTubeSegments(const Protein& protein) {
    std::vector<TubeSegmentSample> out;
    if (!protein.HasCaTubeSegments()) {
        return out;
    }
    const auto& atoms = protein.GetAtoms();
    const auto& segs = protein.GetCaTubeSegments();
    const auto& segSs = protein.GetCaTubeSegmentSs();

    std::vector<int> trace;
    auto flushTrace = [&]() {
        if (trace.size() < 2) {
            trace.clear();
            return;
        }
        for (size_t i = 0; i + 1 < trace.size(); ++i) {
            const int i0 = trace[i > 0 ? i - 1 : i];
            const int i1 = trace[i];
            const int i2 = trace[i + 1];
            const int i3 = trace[i + 2 < trace.size() ? i + 2 : i + 1];
            if (i0 < 0 || i1 < 0 || i2 < 0 || i3 < 0 || i0 >= static_cast<int>(atoms.size()) ||
                i1 >= static_cast<int>(atoms.size()) || i2 >= static_cast<int>(atoms.size()) ||
                i3 >= static_cast<int>(atoms.size())) {
                continue;
            }
            const glm::vec3 p0 = atoms[static_cast<size_t>(i0)].position;
            const glm::vec3 p1 = atoms[static_cast<size_t>(i1)].position;
            const glm::vec3 p2 = atoms[static_cast<size_t>(i2)].position;
            const glm::vec3 p3 = atoms[static_cast<size_t>(i3)].position;
            const SSType ss = (i < segSs.size()) ? static_cast<SSType>(segSs[i]) : SSType::Coil;
            constexpr int kSub = 4;
            glm::vec3 prev = CatmullRom(p0, p1, p2, p3, 0.0f);
            for (int s = 1; s <= kSub; ++s) {
                const float t = static_cast<float>(s) / static_cast<float>(kSub);
                const glm::vec3 cur = CatmullRom(p0, p1, p2, p3, t);
                if (glm::distance(prev, cur) > 1.0e-5f) {
                    out.push_back({prev, cur, ss});
                }
                prev = cur;
            }
        }
        trace.clear();
    };

    for (const auto& e : segs) {
        if (trace.empty()) {
            trace.push_back(e.first);
            trace.push_back(e.second);
            continue;
        }
        if (trace.back() == e.first) {
            trace.push_back(e.second);
        } else {
            flushTrace();
            trace.push_back(e.first);
            trace.push_back(e.second);
        }
    }
    flushTrace();
    return out;
}

} // namespace

void Renderer::DestroyGeometryBuffers() {
    if (m_Vao) {
        glDeleteVertexArrays(1, &m_Vao);
        m_Vao = 0;
    }
    if (m_MeshVbo) {
        glDeleteBuffers(1, &m_MeshVbo);
        m_MeshVbo = 0;
    }
    if (m_Ebo) {
        glDeleteBuffers(1, &m_Ebo);
        m_Ebo = 0;
    }
    if (m_InstanceVbo) {
        glDeleteBuffers(1, &m_InstanceVbo);
        m_InstanceVbo = 0;
    }
    if (m_CylVao) {
        glDeleteVertexArrays(1, &m_CylVao);
        m_CylVao = 0;
    }
    if (m_CylMeshVbo) {
        glDeleteBuffers(1, &m_CylMeshVbo);
        m_CylMeshVbo = 0;
    }
    if (m_CylEbo) {
        glDeleteBuffers(1, &m_CylEbo);
        m_CylEbo = 0;
    }
    if (m_CylInstanceVbo) {
        glDeleteBuffers(1, &m_CylInstanceVbo);
        m_CylInstanceVbo = 0;
    }
    m_IndexCount = 0;
    m_CylIndexCount = 0;
}

void Renderer::CreateGeometryBuffers(int sphereStacks, int sphereSlices, int cylinderSlices) {
    std::vector<float> mesh;
    std::vector<unsigned int> idx;
    BuildUvUnitSphere(sphereStacks, sphereSlices, mesh, idx);
    m_IndexCount = static_cast<unsigned int>(idx.size());

    glGenVertexArrays(1, &m_Vao);
    glGenBuffers(1, &m_MeshVbo);
    glGenBuffers(1, &m_Ebo);
    glGenBuffers(1, &m_InstanceVbo);

    glBindVertexArray(m_Vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_MeshVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.size() * sizeof(float)), mesh.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(idx.size() * sizeof(unsigned int)), idx.data(),
                  GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 8, reinterpret_cast<void*>(0));
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 8, reinterpret_cast<void*>(sizeof(float) * 4));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);

    if (m_CylProgram == 0) {
        return;
    }

    std::vector<float> cylMesh;
    std::vector<unsigned int> cylIdx;
    BuildUnitOpenCylinder(cylinderSlices, cylMesh, cylIdx);
    m_CylIndexCount = static_cast<unsigned int>(cylIdx.size());

    glGenVertexArrays(1, &m_CylVao);
    glGenBuffers(1, &m_CylMeshVbo);
    glGenBuffers(1, &m_CylEbo);
    glGenBuffers(1, &m_CylInstanceVbo);

    glBindVertexArray(m_CylVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_CylMeshVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cylMesh.size() * sizeof(float)), cylMesh.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CylEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cylIdx.size() * sizeof(unsigned int)), cylIdx.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_CylInstanceVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    const GLsizei stride = static_cast<GLsizei>(sizeof(float) * 20);
    for (GLuint c = 0; c < 4; ++c) {
        const GLuint loc = 2 + c;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<void*>(static_cast<uintptr_t>(sizeof(float) * 4 * c)));
        glVertexAttribDivisor(loc, 1);
    }
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(sizeof(float) * 16)));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

Renderer::Renderer() {
    try {
        m_Program = CreateProgramFromFiles("shaders/sphere_instanced.vert", "shaders/sphere_instanced.frag");
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        m_Program = 0;
    }

    try {
        m_CylProgram = CreateProgramFromFiles("shaders/cylinder_instanced.vert", "shaders/cylinder_instanced.frag");
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        m_CylProgram = 0;
    }

    m_LodBracket = 0;
    if (m_Program != 0) {
        CreateGeometryBuffers(18, 36, 24);
    }
}

Renderer::~Renderer() {
    DestroyGeometryBuffers();
    if (m_Program) {
        glDeleteProgram(m_Program);
    }
    if (m_CylProgram) {
        glDeleteProgram(m_CylProgram);
    }
}

void Renderer::SetVisualizationMode(VisualizationMode mode) {
    if (m_CurrentMode == mode) {
        return;
    }
    m_CurrentMode = mode;
    m_InstancesDirty = true;
    m_CylinderInstancesDirty = true;
}

void Renderer::SetProtein(const Protein* protein) {
    m_Protein = protein;
    m_InstancesDirty = true;
    m_CylinderInstancesDirty = true;
    if (!protein || !protein->HasAtoms()) {
        if (m_LodBracket != 0 && m_Program != 0) {
            m_LodBracket = 0;
            DestroyGeometryBuffers();
            CreateGeometryBuffers(18, 36, 24);
        }
        return;
    }
    if (m_Program == 0) {
        return;
    }
    const size_t n = protein->GetAtoms().size();
    const int br = n < 4000 ? 0 : (n < 20000 ? 1 : 2);
    if (br == m_LodBracket) {
        return;
    }
    m_LodBracket = br;
    int st = 18;
    int sl = 36;
    int cyl = 24;
    if (br == 1) {
        st = 14;
        sl = 28;
        cyl = 18;
    } else if (br == 2) {
        st = 10;
        sl = 20;
        cyl = 14;
    }
    DestroyGeometryBuffers();
    CreateGeometryBuffers(st, sl, cyl);
}

float Renderer::RadiusScaleForMode(VisualizationMode mode, float vdwRadius) {
    (void)vdwRadius;
    switch (mode) {
        case VisualizationMode::SpaceFilling:
            return 1.0f;
        case VisualizationMode::BallAndStick:
        case VisualizationMode::Ribbon:
        case VisualizationMode::Cartoon:
        default:
            return 0.32f;
    }
}

void Renderer::RebuildInstanceBuffer() {
    if (!m_Protein || !m_Protein->HasAtoms()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        return;
    }

    if (m_CurrentMode == VisualizationMode::Ribbon || m_CurrentMode == VisualizationMode::Cartoon) {
        glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        return;
    }

    const auto& atoms = m_Protein->GetAtoms();
    std::vector<float> instanceData;
    instanceData.reserve(atoms.size() * 8);
    for (const Atom& a : atoms) {
        const float scale = RadiusScaleForMode(m_CurrentMode, a.radius);
        const float r = std::max(0.05f, a.radius * scale);
        const glm::vec3 c = ElementColor(a.element);
        instanceData.push_back(a.position.x);
        instanceData.push_back(a.position.y);
        instanceData.push_back(a.position.z);
        instanceData.push_back(r);
        instanceData.push_back(c.x);
        instanceData.push_back(c.y);
        instanceData.push_back(c.z);
        instanceData.push_back(1.0f);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instanceData.size() * sizeof(float)), instanceData.data(),
                 GL_DYNAMIC_DRAW);
}

void Renderer::RebuildCylinderInstanceBuffer() {
    if (m_CylInstanceVbo == 0) {
        m_CylinderInstanceCount = 0;
        return;
    }
    if (!m_Protein) {
        glBindBuffer(GL_ARRAY_BUFFER, m_CylInstanceVbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        m_CylinderInstanceCount = 0;
        return;
    }

    const auto& atoms = m_Protein->GetAtoms();
    std::vector<float> instanceData;

    if (m_CurrentMode == VisualizationMode::BallAndStick && m_Protein->HasBonds()) {
        const auto& bonds = m_Protein->GetBonds();
        instanceData.reserve(bonds.size() * 20);

        constexpr float kBallScale = 0.32f;
        constexpr float kStickRadius = 0.18f;

        for (const auto& e : bonds) {
            const int ia = e.first;
            const int ib = e.second;
            if (ia < 0 || ib < 0 || ia >= static_cast<int>(atoms.size()) || ib >= static_cast<int>(atoms.size())) {
                continue;
            }
            const Atom& A = atoms[static_cast<size_t>(ia)];
            const Atom& B = atoms[static_cast<size_t>(ib)];
            const glm::vec3 delta = B.position - A.position;
            const float fullLen = glm::length(delta);
            if (fullLen < 1.0e-4f) {
                continue;
            }
            const glm::vec3 dir = delta / fullLen;
            const float rA = std::max(0.05f, A.radius * kBallScale);
            const float rB = std::max(0.05f, B.radius * kBallScale);
            float cylLen = fullLen - rA - rB;
            if (cylLen < 0.02f) {
                cylLen = 0.02f;
            }
            const glm::vec3 base = A.position + dir * rA;

            glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            if (std::abs(glm::dot(worldUp, dir)) > 0.95f) {
                worldUp = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            const glm::vec3 xax = glm::normalize(glm::cross(worldUp, dir));
            const glm::vec3 zax = glm::cross(dir, xax);
            glm::mat4 R(1.0f);
            R[0] = glm::vec4(xax, 0.0f);
            R[1] = glm::vec4(dir, 0.0f);
            R[2] = glm::vec4(zax, 0.0f);
            R[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

            const glm::mat4 M = glm::translate(glm::mat4(1.0f), base) * R *
                                 glm::scale(glm::mat4(1.0f), glm::vec3(kStickRadius, cylLen, kStickRadius));

            const glm::vec3 ca = ElementColor(A.element);
            const glm::vec3 cb = ElementColor(B.element);
            const glm::vec3 blend = 0.5f * (ca + cb);
            AppendMat4Color(instanceData, M, glm::vec4(blend, 1.0f));
        }
    } else if ((m_CurrentMode == VisualizationMode::Ribbon || m_CurrentMode == VisualizationMode::Cartoon) &&
               m_Protein->HasCaTubeSegments()) {
        const std::vector<TubeSegmentSample> smoothSegs = BuildSmoothedTubeSegments(*m_Protein);
        instanceData.reserve(smoothSegs.size() * 20);
        const float tubeRadius = (m_CurrentMode == VisualizationMode::Cartoon) ? 0.36f : 0.30f;

        for (const TubeSegmentSample& seg : smoothSegs) {
            const glm::vec3 delta = seg.b - seg.a;
            const float fullLen = glm::length(delta);
            if (fullLen < 1.0e-4f) {
                continue;
            }
            const glm::vec3 dir = delta / fullLen;
            const glm::vec3 base = seg.a;

            glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            if (std::abs(glm::dot(worldUp, dir)) > 0.95f) {
                worldUp = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            const glm::vec3 xax = glm::normalize(glm::cross(worldUp, dir));
            const glm::vec3 zax = glm::cross(dir, xax);
            glm::mat4 R(1.0f);
            R[0] = glm::vec4(xax, 0.0f);
            R[1] = glm::vec4(dir, 0.0f);
            R[2] = glm::vec4(zax, 0.0f);
            R[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

            const SSType ss = seg.ss;

            float sx = tubeRadius;
            float sy = fullLen;
            float sz = tubeRadius;
            if (m_CurrentMode == VisualizationMode::Cartoon) {
                if (ss == SSType::Strand) {
                    sx = tubeRadius * 2.65f;
                    sz = tubeRadius * 0.34f;
                } else if (ss == SSType::Helix) {
                    sx = tubeRadius * 1.15f;
                    sz = tubeRadius * 1.15f;
                }
            }

            const glm::mat4 M =
                glm::translate(glm::mat4(1.0f), base) * R * glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));

            const glm::vec3 col = SsColor(ss);
            AppendMat4Color(instanceData, M, glm::vec4(col, 1.0f));
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_CylInstanceVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instanceData.size() * sizeof(float)), instanceData.data(),
                 GL_DYNAMIC_DRAW);
    m_CylinderInstanceCount = static_cast<int>(instanceData.size() / 20);
}

void Renderer::EnsureGpuReady() {
    if (m_Program == 0 || m_Vao == 0) {
        return;
    }
    if (m_InstancesDirty) {
        RebuildInstanceBuffer();
        m_InstancesDirty = false;
    }
    if (m_CylinderInstancesDirty) {
        RebuildCylinderInstanceBuffer();
        m_CylinderInstancesDirty = false;
    }
}

void Renderer::Draw(int viewportWidth, int viewportHeight, const glm::mat4& view, const glm::mat4& projection) {
    (void)viewportWidth;
    (void)viewportHeight;
    if (m_Program == 0 || m_Vao == 0) {
        return;
    }
    EnsureGpuReady();
    if (!m_Protein || !m_Protein->HasAtoms()) {
        return;
    }

    glUseProgram(m_Program);
    const GLint locView = glGetUniformLocation(m_Program, "uView");
    const GLint locProj = glGetUniformLocation(m_Program, "uProjection");
    const GLint locLight = glGetUniformLocation(m_Program, "uLightPos");
    glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(locProj, 1, GL_FALSE, &projection[0][0]);
    const glm::vec3 lightPos(8.0f, 12.0f, 10.0f);
    glUniform3f(locLight, lightPos.x, lightPos.y, lightPos.z);

    glBindVertexArray(m_Vao);
    const GLsizei instanceCount = static_cast<GLsizei>(m_Protein->GetAtoms().size());
    if (instanceCount > 0) {
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_IndexCount), GL_UNSIGNED_INT, nullptr,
                                instanceCount);
    }
    glBindVertexArray(0);

    const bool wantCylinders =
        (m_CurrentMode == VisualizationMode::BallAndStick || m_CurrentMode == VisualizationMode::Ribbon ||
         m_CurrentMode == VisualizationMode::Cartoon);
    if (wantCylinders && m_CylProgram != 0 && m_CylVao != 0 && m_CylinderInstanceCount > 0) {
        glUseProgram(m_CylProgram);
        const GLint clView = glGetUniformLocation(m_CylProgram, "uView");
        const GLint clProj = glGetUniformLocation(m_CylProgram, "uProjection");
        const GLint clLight = glGetUniformLocation(m_CylProgram, "uLightPos");
        glUniformMatrix4fv(clView, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(clProj, 1, GL_FALSE, &projection[0][0]);
        const glm::vec3 lightPos(8.0f, 12.0f, 10.0f);
        glUniform3f(clLight, lightPos.x, lightPos.y, lightPos.z);
        glBindVertexArray(m_CylVao);
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_CylIndexCount), GL_UNSIGNED_INT, nullptr,
                                static_cast<GLsizei>(m_CylinderInstanceCount));
        glBindVertexArray(0);
    }

    glUseProgram(0);
}
