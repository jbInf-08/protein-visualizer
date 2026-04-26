#pragma once

#include "Atom.h"
#include "SecondaryStructure.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class Protein {
public:
    void AddAtom(const Atom& atom);
    void SetAtoms(std::vector<Atom> atoms);
    const std::vector<Atom>& GetAtoms() const { return m_Atoms; }

    void SetName(const std::string& name) { m_Name = name; }
    const std::string& GetName() const { return m_Name; }

    bool HasAtoms() const { return !m_Atoms.empty(); }
    std::pair<glm::vec3, glm::vec3> GetAxisAlignedBounds() const;
    glm::vec3 GetCenter() const;
    float GetBoundingRadius() const;

    void SetBonds(std::vector<std::pair<int, int>> bonds);
    const std::vector<std::pair<int, int>>& GetBonds() const { return m_Bonds; }
    bool HasBonds() const { return !m_Bonds.empty(); }

    void SetCaTubeSegments(std::vector<std::pair<int, int>> segments);
    const std::vector<std::pair<int, int>>& GetCaTubeSegments() const { return m_CaTubeSegments; }
    bool HasCaTubeSegments() const { return !m_CaTubeSegments.empty(); }

    void SetAtomSecondaryStructure(std::vector<std::uint8_t> ss);
    const std::vector<std::uint8_t>& GetAtomSecondaryStructure() const { return m_AtomSs; }
    SSType GetAtomSSType(size_t atomIndex) const;

    void SetCaTubeSegmentSs(std::vector<std::uint8_t> segSs);
    const std::vector<std::uint8_t>& GetCaTubeSegmentSs() const { return m_CaTubeSegmentSs; }

private:
    std::vector<Atom> m_Atoms;
    std::vector<std::pair<int, int>> m_Bonds;
    std::vector<std::pair<int, int>> m_CaTubeSegments;
    std::vector<std::uint8_t> m_AtomSs;
    std::vector<std::uint8_t> m_CaTubeSegmentSs;
    std::string m_Name;
};
