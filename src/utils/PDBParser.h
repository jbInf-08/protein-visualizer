#pragma once
#include "../model/Protein.h"
class PDBParser {
public:
    static Protein ParsePDBFile(const std::string& filepath);
}; 