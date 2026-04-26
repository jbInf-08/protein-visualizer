#include "Protein.h"

#include <algorithm>
#include <cmath>
#include <limits>

void Protein::AddAtom(const Atom& atom) {
    m_Atoms.push_back(atom);
}

void Protein::SetAtoms(std::vector<Atom> atoms) {
    m_Atoms = std::move(atoms);
}

void Protein::SetBonds(std::vector<std::pair<int, int>> bonds) {
    m_Bonds = std::move(bonds);
}

void Protein::SetCaTubeSegments(std::vector<std::pair<int, int>> segments) {
    m_CaTubeSegments = std::move(segments);
}

void Protein::SetAtomSecondaryStructure(std::vector<std::uint8_t> ss) {
    m_AtomSs = std::move(ss);
}

SSType Protein::GetAtomSSType(size_t atomIndex) const {
    if (atomIndex >= m_AtomSs.size()) {
        return SSType::Coil;
    }
    const auto v = static_cast<SSType>(m_AtomSs[atomIndex]);
    if (v == SSType::Helix || v == SSType::Strand || v == SSType::Coil) {
        return v;
    }
    return SSType::Coil;
}

void Protein::SetCaTubeSegmentSs(std::vector<std::uint8_t> segSs) {
    m_CaTubeSegmentSs = std::move(segSs);
}

std::pair<glm::vec3, glm::vec3> Protein::GetAxisAlignedBounds() const {
    if (m_Atoms.empty()) {
        return {glm::vec3(0.0f), glm::vec3(0.0f)};
    }
    glm::vec3 lo(std::numeric_limits<float>::max());
    glm::vec3 hi(-std::numeric_limits<float>::max());
    for (const Atom& a : m_Atoms) {
        lo = glm::min(lo, a.position);
        hi = glm::max(hi, a.position);
    }
    return {lo, hi};
}

glm::vec3 Protein::GetCenter() const {
    const auto [lo, hi] = GetAxisAlignedBounds();
    return (lo + hi) * 0.5f;
}

float Protein::GetBoundingRadius() const {
    if (m_Atoms.empty()) {
        return 0.0f;
    }
    const glm::vec3 c = GetCenter();
    float r = 0.0f;
    for (const Atom& a : m_Atoms) {
        const float d = glm::length(a.position - c);
        r = std::max(r, d);
    }
    return r;
}
