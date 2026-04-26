#pragma once

#include "../model/Protein.h"

#include <string>

/// Minimal mmCIF reader for the atom_site loop (no external dependency).
class MmCifParser {
public:
    static Protein ParseFile(const std::string& filepath);
};
