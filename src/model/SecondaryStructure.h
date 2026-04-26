#pragma once

#include <cstdint>

/// Secondary structure type from PDB HELIX / SHEET (no DSSP).
enum class SSType : std::uint8_t { Coil = 0, Strand = 1, Helix = 2 };

inline int SsPriority(SSType s) {
    switch (s) {
        case SSType::Strand:
            return 3;
        case SSType::Helix:
            return 2;
        default:
            return 1;
    }
}

inline SSType SsMerge(SSType a, SSType b) {
    return SsPriority(a) >= SsPriority(b) ? a : b;
}
