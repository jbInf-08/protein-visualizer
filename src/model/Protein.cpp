#include "Protein.h"
void Protein::AddAtom(const Atom& atom) { m_Atoms.push_back(atom); }
const std::vector<Atom>& Protein::GetAtoms() const { return m_Atoms; } 