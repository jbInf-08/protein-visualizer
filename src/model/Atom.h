#pragma once
#include <glm/glm.hpp>
#include <string>

struct Atom {
    glm::vec3 position;
    std::string element;
    float radius;
    int chainId;
    /// PDB ATOM/HETATM serial number (1-based in file); -1 if unknown.
    int serial = -1;
    /// PDB atom name (e.g. "CA", "N"); may include padding from the file.
    std::string atomName;
    /// Three-letter residue name when present.
    std::string residueName;
    int residueSeq = 0;
    char insertionCode = ' ';
    char chainChar = ' ';
    /// True if this record was HETATM (ligand/solvent); standard polymer ATOM is false.
    bool hetero = false;
    /// PDB alternate location indicator (column 17); space or '.' if none.
    char altLoc = ' ';
};
