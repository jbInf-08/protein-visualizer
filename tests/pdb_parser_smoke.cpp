#include "utils/StructureLoader.h"

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: parser_smoke_test <pdb-path>\n";
        return 2;
    }
    const std::string path = argv[1];
    Protein protein = StructureLoader::Load(path);
    const auto& atoms = protein.GetAtoms();
    if (atoms.size() < 300) {
        std::cerr << "Unexpected atom count: " << atoms.size() << "\n";
        return 1;
    }
    std::cout << "Loaded atoms: " << atoms.size() << "\n";
    return 0;
}
