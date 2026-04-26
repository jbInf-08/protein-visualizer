#include "StructureLoader.h"

#include "MmCifParser.h"
#include "PDBParser.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>

namespace {

bool EndsWithCif(const std::string& path) {
    const std::filesystem::path p(path);
    std::string ext = p.extension().string();
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext == ".cif" || ext == ".mmcif";
}

} // namespace

Protein StructureLoader::Load(const std::string& filepath) {
    if (filepath.empty()) {
        throw std::runtime_error("Empty path.");
    }
    if (EndsWithCif(filepath)) {
        return MmCifParser::ParseFile(filepath);
    }
    return PDBParser::ParsePDBFile(filepath);
}
