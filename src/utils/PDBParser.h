#pragma once

#include "../model/Atom.h"
#include "../model/Protein.h"
#include "../model/SecondaryStructure.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

struct SecondaryStructureRange {
    char chain = ' ';
    int startSeq = 0;
    int endSeq = 0;
    SSType type = SSType::Coil;
};

class PDBParser {
public:
    /// Enables stricter DSSP residue labeling (E/B/G/I/T/H) prior to H/S/C rendering map.
    static void SetStrictDsspMode(bool enabled);

    static Protein ParsePDBFile(const std::string& filepath);

    /// After atoms are populated (PDB or mmCIF), assign CONECT/bonds, secondary structure, and Cα trace.
    static void FinalizeStructure(Protein& protein, const std::set<std::pair<int, int>>& serialEdges,
                                  const std::vector<std::string>& helixLines, const std::vector<std::string>& sheetLines,
                                  const std::vector<SecondaryStructureRange>& mmcifRanges = {});

    /// Same altLoc / site deduplication policy as PDB ATOM records (used by mmCIF loader).
    static void ApplyAtomSiteAltLoc(std::vector<Atom>& rawAtoms, Protein& protein);
};
