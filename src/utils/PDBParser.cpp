#include "PDBParser.h"
#include <fstream>
#include <sstream>

Protein PDBParser::ParsePDBFile(const std::string& filepath) {
    Protein protein;
    std::ifstream file(filepath);
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 4) == "ATOM") {
            Atom atom;
            atom.position.x = std::stof(line.substr(30, 8));
            atom.position.y = std::stof(line.substr(38, 8));
            atom.position.z = std::stof(line.substr(46, 8));
            atom.element = line.substr(76, 2);
            protein.AddAtom(atom);
        }
    }
    return protein;
} 