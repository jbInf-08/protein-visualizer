#include "Picking.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Picking {
namespace {

glm::vec3 ScreenRayDir(float mouseX, float mouseY, int fbW, int fbH, const glm::mat4& invVp, glm::vec3* rayOrigin) {
    const float w = static_cast<float>(std::max(1, fbW));
    const float h = static_cast<float>(std::max(1, fbH));
    const float xNdc = (2.0f * mouseX / w) - 1.0f;
    const float yNdc = 1.0f - (2.0f * mouseY / h);
    glm::vec4 pNear = invVp * glm::vec4(xNdc, yNdc, -1.0f, 1.0f);
    glm::vec4 pFar = invVp * glm::vec4(xNdc, yNdc, 1.0f, 1.0f);
    pNear /= pNear.w;
    pFar /= pFar.w;
    *rayOrigin = glm::vec3(pNear);
    glm::vec3 dir = glm::vec3(pFar - pNear);
    const float len = glm::length(dir);
    if (len < 1.0e-8f) {
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }
    return dir / len;
}

bool RaySphereFirstHit(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& center, float radius, float* outT) {
    if (radius <= 0.0f) {
        return false;
    }
    const glm::vec3 oc = ro - center;
    const float b = glm::dot(oc, rd);
    const float c = glm::dot(oc, oc) - radius * radius;
    const float disc = b * b - c;
    if (disc < 0.0f) {
        return false;
    }
    const float s = std::sqrt(disc);
    float t = -b - s;
    if (t < 0.0f) {
        t = -b + s;
    }
    if (t < 0.0f) {
        return false;
    }
    *outT = t;
    return true;
}

glm::vec3 CatmullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

bool RayCapsuleFirstHit(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& a, const glm::vec3& b, float radius,
                        float* outT) {
    const glm::vec3 ba = b - a;
    const glm::vec3 oa = ro - a;
    const float baba = glm::dot(ba, ba);
    if (baba < 1.0e-8f) {
        return RaySphereFirstHit(ro, rd, a, radius, outT);
    }
    const float bard = glm::dot(ba, rd);
    const float baoa = glm::dot(ba, oa);
    const float rdoa = glm::dot(rd, oa);
    const float oaoa = glm::dot(oa, oa);

    const float A = baba - bard * bard;
    const float B = baba * rdoa - baoa * bard;
    const float C = baba * oaoa - baoa * baoa - radius * radius * baba;

    float tBest = std::numeric_limits<float>::infinity();
    bool hit = false;
    const float h = B * B - A * C;
    if (h >= 0.0f && std::abs(A) > 1.0e-8f) {
        const float t = (-B - std::sqrt(h)) / A;
        const float y = baoa + t * bard;
        if (t >= 0.0f && y >= 0.0f && y <= baba) {
            tBest = t;
            hit = true;
        }
    }

    float ts = 0.0f;
    if (RaySphereFirstHit(ro, rd, a, radius, &ts) && ts < tBest) {
        tBest = ts;
        hit = true;
    }
    if (RaySphereFirstHit(ro, rd, b, radius, &ts) && ts < tBest) {
        tBest = ts;
        hit = true;
    }
    if (hit) {
        *outT = tBest;
    }
    return hit;
}

struct SmoothSeg {
    int atomIndex = -1;
    glm::vec3 a{0.0f};
    glm::vec3 b{0.0f};
};

std::vector<SmoothSeg> BuildSmoothSegments(const Protein& protein) {
    std::vector<SmoothSeg> out;
    if (!protein.HasCaTubeSegments()) {
        return out;
    }
    const auto& atoms = protein.GetAtoms();
    const auto& segs = protein.GetCaTubeSegments();
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
            constexpr int kSub = 4;
            glm::vec3 prev = CatmullRom(p0, p1, p2, p3, 0.0f);
            for (int s = 1; s <= kSub; ++s) {
                const float t = static_cast<float>(s) / static_cast<float>(kSub);
                const glm::vec3 cur = CatmullRom(p0, p1, p2, p3, t);
                if (glm::distance(prev, cur) > 1.0e-5f) {
                    out.push_back({i1, prev, cur});
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

std::optional<int> PickAtomIndexScreen(const Protein& protein, float mouseX, float mouseY, int framebufferWidth,
                                       int framebufferHeight, const glm::mat4& view, const glm::mat4& projection,
                                       VisualizationMode mode) {
    const glm::mat4 vp = projection * view;
    const glm::mat4 invVp = glm::inverse(vp);
    glm::vec3 ro{};
    const glm::vec3 rd = ScreenRayDir(mouseX, mouseY, framebufferWidth, framebufferHeight, invVp, &ro);

    float bestT = std::numeric_limits<float>::infinity();
    int bestIndex = -1;

    const auto& atoms = protein.GetAtoms();
    for (int i = 0; i < static_cast<int>(atoms.size()); ++i) {
        const Atom& a = atoms[static_cast<size_t>(i)];
        const float scale = Renderer::RadiusScaleForMode(mode, a.radius);
        const float r = std::max(0.05f, a.radius * scale);
        float t = 0.0f;
        if (RaySphereFirstHit(ro, rd, a.position, r, &t) && t < bestT) {
            bestT = t;
            bestIndex = i;
        }
    }

    if (mode == VisualizationMode::BallAndStick && protein.HasBonds()) {
        constexpr float kStickRadius = 0.18f;
        constexpr float kBallScale = 0.32f;
        for (const auto& e : protein.GetBonds()) {
            const int ia = e.first;
            const int ib = e.second;
            if (ia < 0 || ib < 0 || ia >= static_cast<int>(atoms.size()) || ib >= static_cast<int>(atoms.size())) {
                continue;
            }
            const Atom& A = atoms[static_cast<size_t>(ia)];
            const Atom& B = atoms[static_cast<size_t>(ib)];
            const glm::vec3 delta = B.position - A.position;
            const float len = glm::length(delta);
            if (len < 1.0e-5f) {
                continue;
            }
            const glm::vec3 dir = delta / len;
            const float rA = std::max(0.05f, A.radius * kBallScale);
            const float rB = std::max(0.05f, B.radius * kBallScale);
            const glm::vec3 p0 = A.position + dir * rA;
            const glm::vec3 p1 = B.position - dir * rB;
            float t = 0.0f;
            if (RayCapsuleFirstHit(ro, rd, p0, p1, kStickRadius, &t) && t < bestT) {
                bestT = t;
                bestIndex = ia;
            }
        }
    }

    if (mode == VisualizationMode::Ribbon || mode == VisualizationMode::Cartoon) {
        const float tubePickR = (mode == VisualizationMode::Cartoon) ? 0.36f : 0.30f;
        const std::vector<SmoothSeg> segs = BuildSmoothSegments(protein);
        for (const SmoothSeg& seg : segs) {
            float t = 0.0f;
            if (RayCapsuleFirstHit(ro, rd, seg.a, seg.b, tubePickR, &t) && t < bestT) {
                bestT = t;
                bestIndex = seg.atomIndex;
            }
        }
    }

    if (bestIndex < 0) {
        return std::nullopt;
    }
    return bestIndex;
}

} // namespace Picking
