#include "MmCifParser.h"

#include "PDBParser.h"

#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

std::string Trim(const std::string& s) {
    size_t a = 0;
    size_t b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) {
        ++a;
    }
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) {
        --b;
    }
    return s.substr(a, b - a);
}

std::vector<std::string> SplitCifLine(const std::string& line) {
    std::vector<std::string> tok;
    std::string cur;
    bool inQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '\'') {
            inQuote = !inQuote;
            continue;
        }
        if (!inQuote && std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) {
                tok.push_back(cur);
                cur.clear();
            }
            continue;
        }
        cur += c;
    }
    if (!cur.empty()) {
        tok.push_back(cur);
    }
    return tok;
}

int ParseIntLoose(const std::string& s) {
    const std::string t = Trim(s);
    if (t.empty() || t == "?" || t == ".") {
        return 0;
    }
    try {
        return std::stoi(t);
    } catch (...) {
        return 0;
    }
}

float ParseFloatLoose(const std::string& s) {
    const std::string t = Trim(s);
    if (t.empty() || t == "?" || t == ".") {
        return 0.0f;
    }
    try {
        return std::stof(t);
    } catch (...) {
        return 0.0f;
    }
}

float VdwRadiusForElement(const std::string& elem) {
    if (elem.empty()) {
        return 1.5f;
    }
    const char e = static_cast<char>(std::toupper(static_cast<unsigned char>(elem[0])));
    if (elem.size() >= 2 && e == 'C' && std::toupper(static_cast<unsigned char>(elem[1])) == 'L') {
        return 1.7f;
    }
    switch (e) {
        case 'H':
            return 1.2f;
        case 'C':
            return 1.7f;
        case 'N':
            return 1.55f;
        case 'O':
            return 1.52f;
        case 'S':
            return 1.8f;
        case 'P':
            return 1.8f;
        case 'F':
            return 1.47f;
        default:
            return 1.5f;
    }
}

std::string ElementFromMmCifRow(const std::unordered_map<std::string, int>& col,
                                  const std::vector<std::string>& fields) {
    const auto itSym = col.find("type_symbol");
    if (itSym != col.end()) {
        const int k = itSym->second;
        if (k >= 0 && static_cast<size_t>(k) < fields.size()) {
            const std::string s = Trim(fields[static_cast<size_t>(k)]);
            if (!s.empty() && s != "?" && s != ".") {
                std::string u = s;
                for (char& c : u) {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                }
                return u;
            }
        }
    }
    const auto itName = col.find("label_atom_id");
    if (itName != col.end()) {
        const int k = itName->second;
        if (k >= 0 && static_cast<size_t>(k) < fields.size()) {
            const std::string an = Trim(fields[static_cast<size_t>(k)]);
            if (!an.empty()) {
                return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(an[0]))));
            }
        }
    }
    return "C";
}

void ParseAtomSiteChunk(const std::string& chunk, std::vector<Atom>& rawAtoms) {
    if (chunk.find("_atom_site.") == std::string::npos) {
        return;
    }

    std::vector<std::string> colnames;
    std::istringstream ss(chunk);
    std::string line;
    while (std::getline(ss, line)) {
        const std::string t = Trim(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }
        if (t == "loop_") {
            continue;
        }
        if (t.rfind("_atom_site.", 0) == 0) {
            colnames.push_back(t.substr(std::string("_atom_site.").size()));
            continue;
        }
        if (t[0] == '_') {
            break;
        }
        if (colnames.empty()) {
            continue;
        }

        const std::vector<std::string> fields = SplitCifLine(t);
        if (fields.size() < colnames.size()) {
            continue;
        }

        std::unordered_map<std::string, int> col;
        for (size_t i = 0; i < colnames.size(); ++i) {
            col[colnames[i]] = static_cast<int>(i);
        }

        const auto need = [&](const char* name) -> int {
            const auto it = col.find(name);
            return it == col.end() ? -1 : it->second;
        };
        const int iGx = need("Cartn_x");
        const int iGy = need("Cartn_y");
        const int iGz = need("Cartn_z");
        const int iAtom = need("label_atom_id");
        const int iRes = need("label_comp_id");
        const int iChain = need("label_asym_id");
        if (iGx < 0 || iGy < 0 || iGz < 0 || iAtom < 0 || iRes < 0 || iChain < 0) {
            continue;
        }

        Atom a{};
        a.position.x = ParseFloatLoose(fields[static_cast<size_t>(iGx)]);
        a.position.y = ParseFloatLoose(fields[static_cast<size_t>(iGy)]);
        a.position.z = ParseFloatLoose(fields[static_cast<size_t>(iGz)]);
        a.atomName = Trim(fields[static_cast<size_t>(iAtom)]);
        a.residueName = Trim(fields[static_cast<size_t>(iRes)]);
        const std::string chainTok = Trim(fields[static_cast<size_t>(iChain)]);
        a.chainChar = chainTok.empty() ? ' ' : chainTok[0];
        a.chainId = static_cast<int>(static_cast<unsigned char>(a.chainChar));

        const int iSeq = need("label_seq_id");
        if (iSeq >= 0 && static_cast<size_t>(iSeq) < fields.size()) {
            a.residueSeq = ParseIntLoose(fields[static_cast<size_t>(iSeq)]);
        }

        const int iHet = need("group_PDB");
        if (iHet >= 0 && static_cast<size_t>(iHet) < fields.size()) {
            const std::string g = Trim(fields[static_cast<size_t>(iHet)]);
            a.hetero = (g == "HETATM");
        }

        const int iId = need("id");
        if (iId >= 0 && static_cast<size_t>(iId) < fields.size()) {
            const int sid = ParseIntLoose(fields[static_cast<size_t>(iId)]);
            a.serial = sid > 0 ? sid : -1;
        } else {
            a.serial = static_cast<int>(rawAtoms.size()) + 1;
        }

        const int iAlt = need("label_alt_id");
        if (iAlt >= 0 && static_cast<size_t>(iAlt) < fields.size()) {
            const std::string al = Trim(fields[static_cast<size_t>(iAlt)]);
            const char c = al.empty() ? ' ' : al[0];
            a.altLoc = (c == '.' || c == '?') ? ' ' : c;
        }

        const int iIcode = need("pdbx_PDB_ins_code");
        if (iIcode >= 0 && static_cast<size_t>(iIcode) < fields.size()) {
            const std::string ic = Trim(fields[static_cast<size_t>(iIcode)]);
            a.insertionCode = ic.empty() ? ' ' : ic[0];
        }

        a.element = ElementFromMmCifRow(col, fields);
        a.radius = VdwRadiusForElement(a.element);

        rawAtoms.push_back(std::move(a));
    }
}

SSType StructConfTypeFromId(const std::string& confType) {
    std::string u = Trim(confType);
    for (char& c : u) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    if (u.find("HELX") != std::string::npos || u.find("HELIX") != std::string::npos) {
        return SSType::Helix;
    }
    if (u.find("STRN") != std::string::npos || u.find("SHEET") != std::string::npos ||
        u.find("BETA") != std::string::npos) {
        return SSType::Strand;
    }
    return SSType::Coil;
}

void ParseStructConfChunk(const std::string& chunk, std::vector<SecondaryStructureRange>& ranges) {
    if (chunk.find("_struct_conf.") == std::string::npos) {
        return;
    }

    std::vector<std::string> colnames;
    std::istringstream ss(chunk);
    std::string line;
    while (std::getline(ss, line)) {
        const std::string t = Trim(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }
        if (t == "loop_") {
            continue;
        }
        if (t.rfind("_struct_conf.", 0) == 0) {
            colnames.push_back(t.substr(std::string("_struct_conf.").size()));
            continue;
        }
        if (t[0] == '_') {
            break;
        }
        if (colnames.empty()) {
            continue;
        }

        const std::vector<std::string> fields = SplitCifLine(t);
        if (fields.size() < colnames.size()) {
            continue;
        }

        std::unordered_map<std::string, int> col;
        for (size_t i = 0; i < colnames.size(); ++i) {
            col[colnames[i]] = static_cast<int>(i);
        }

        const auto need = [&](const char* name) -> int {
            const auto it = col.find(name);
            return it == col.end() ? -1 : it->second;
        };
        const int iType = need("conf_type_id");
        const int iBegAsym = need("beg_label_asym_id");
        const int iEndAsym = need("end_label_asym_id");
        const int iBegSeq = need("beg_label_seq_id");
        const int iEndSeq = need("end_label_seq_id");
        if (iType < 0 || iBegAsym < 0 || iEndAsym < 0 || iBegSeq < 0 || iEndSeq < 0) {
            continue;
        }

        const SSType type = StructConfTypeFromId(fields[static_cast<size_t>(iType)]);
        if (type == SSType::Coil) {
            continue;
        }
        const std::string begChain = Trim(fields[static_cast<size_t>(iBegAsym)]);
        const std::string endChain = Trim(fields[static_cast<size_t>(iEndAsym)]);
        if (begChain.empty() || endChain.empty() || begChain[0] != endChain[0]) {
            continue;
        }
        const int begSeq = ParseIntLoose(fields[static_cast<size_t>(iBegSeq)]);
        const int endSeq = ParseIntLoose(fields[static_cast<size_t>(iEndSeq)]);
        if (begSeq == 0 || endSeq == 0) {
            continue;
        }
        SecondaryStructureRange r{};
        r.chain = begChain[0];
        r.startSeq = begSeq;
        r.endSeq = endSeq;
        r.type = type;
        ranges.push_back(r);
    }
}

} // namespace

Protein MmCifParser::ParseFile(const std::string& filepath) {
    Protein protein;
    protein.SetName(filepath);

    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open mmCIF file: " + filepath);
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    const std::string all = buf.str();

    std::vector<Atom> rawAtoms;
    rawAtoms.reserve(8192);
    std::vector<SecondaryStructureRange> ssRanges;

    size_t pos = 0;
    while (pos < all.size()) {
        const size_t lp = all.find("loop_", pos);
        if (lp == std::string::npos) {
            break;
        }
        size_t chunkEnd = all.find("\nloop_", lp + 5);
        if (chunkEnd == std::string::npos) {
            chunkEnd = all.size();
        } else {
            ++chunkEnd;
        }
        const std::string chunk = all.substr(lp, chunkEnd - lp);
        if (chunk.find("_atom_site.") != std::string::npos) {
            ParseAtomSiteChunk(chunk, rawAtoms);
        }
        if (chunk.find("_struct_conf.") != std::string::npos) {
            ParseStructConfChunk(chunk, ssRanges);
        }
        pos = chunkEnd;
    }

    if (rawAtoms.empty()) {
        throw std::runtime_error("No _atom_site data found in mmCIF: " + filepath);
    }

    PDBParser::ApplyAtomSiteAltLoc(rawAtoms, protein);
    const std::set<std::pair<int, int>> noConect;
    const std::vector<std::string> noHelix;
    const std::vector<std::string> noSheet;
    PDBParser::FinalizeStructure(protein, noConect, noHelix, noSheet, ssRanges);
    return protein;
}
