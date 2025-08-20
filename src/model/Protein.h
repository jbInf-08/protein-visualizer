#pragma once
#include "Atom.h"
#include <vector>
#include <string>
class Protein {
    std::vector<Atom> m_Atoms;
    std::string m_Name;
public:
    void AddAtom(const Atom& atom);
    const std::vector<Atom>& GetAtoms() const;
    void SetName(const std::string& name) { m_Name = name; }
    const std::string& GetName() const { return m_Name; }
}; 