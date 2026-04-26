#pragma once

#include "../model/Protein.h"

#include <string>

/// Dispatches by file extension to PDB or mmCIF parsers.
class StructureLoader {
public:
    static Protein Load(const std::string& filepath);
};
