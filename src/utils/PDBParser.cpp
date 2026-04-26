#include "PDBParser.h"
#include "BondDistanceConstants.h"

#include "../model/SecondaryStructure.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace detail {

bool g_StrictDsspMode = true;

std::string TrimToken(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

int ParseIntField(const std::string& line, size_t start, size_t len) {
    if (start + len > line.size()) {
        return 0;
    }
    const std::string s = TrimToken(line.substr(start, len));
    if (s.empty()) {
        return 0;
    }
    try {
        return std::stoi(s);
    } catch (...) {
        return 0;
    }
}

float VdwRadiusForElement(const std::string& elem) {
    if (elem.empty()) {
        return 1.5f;
    }
    const char e = static_cast<char>(std::toupper(static_cast<unsigned char>(elem[0])));
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
        default:
            return 1.5f;
    }
}

float CovalentRadiusAngstrom(const std::string& elem) {
    if (elem.empty()) {
        return 0.8f;
    }
    std::string u = TrimToken(elem);
    for (char& c : u) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    if (u.size() >= 2) {
        if (u[0] == 'C' && u[1] == 'L') {
            return 1.02f;
        }
        if (u[0] == 'B' && u[1] == 'R') {
            return 1.20f;
        }
        if (u == "FE") {
            return 1.25f;
        }
        if (u == "ZN") {
            return 1.22f;
        }
        if (u == "CU") {
            return 1.32f;
        }
        if (u == "MG") {
            return 1.30f;
        }
        if (u == "MN") {
            return 1.39f;
        }
        if (u == "MO") {
            return 1.54f;
        }
        if (u == "CO") {
            return 1.50f;
        }
        if (u == "NI") {
            return 1.24f;
        }
        if (u == "CD") {
            return 1.44f;
        }
        if (u == "HG") {
            return 1.32f;
        }
        if (u == "AU") {
            return 1.44f;
        }
        if (u == "NA") {
            return 1.66f;
        }
        if (u == "CA" && u.size() == 2) {
            return 1.74f;
        }
        if (u == "K") {
            return 2.03f;
        }
        if (u == "SR") {
            return 1.92f;
        }
        if (u == "BA") {
            return 1.98f;
        }
        if (u == "PB") {
            return 1.46f;
        }
    }
    const char e = u[0];
    switch (e) {
        case 'H':
            return 0.31f;
        case 'C':
            return 0.76f;
        case 'N':
            return 0.71f;
        case 'O':
            return 0.66f;
        case 'S':
            return 1.05f;
        case 'P':
            return 1.07f;
        case 'F':
            return 0.57f;
        default:
            return 0.8f;
    }
}

char ElementKeyChar(const std::string& elem) {
    if (elem.empty()) {
        return 'X';
    }
    return static_cast<char>(std::toupper(static_cast<unsigned char>(elem[0])));
}

bool IsMetalElement(const std::string& elem) {
    std::string u = TrimToken(elem);
    for (char& c : u) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    if (u.size() >= 2) {
        if (u == "FE" || u == "ZN" || u == "CU" || u == "MG" || u == "MN" || u == "MO" || u == "CO" || u == "NI" ||
            u == "CD" || u == "HG" || u == "AU" || u == "NA" || u == "SR" || u == "BA" || u == "PB" || u == "LI" ||
            u == "RB" || u == "CS" || u == "AL" || u == "CR" || u == "AG" || u == "PT" || u == "PD" || u == "IR" ||
            u == "RU" || u == "TI" || u == "V" || u == "W") {
            return true;
        }
        if (u == "CA") {
            return true;
        }
    }
    if (u.size() == 1 && u[0] == 'K') {
        return true;
    }
    return false;
}

float MaxAllowedBondDistance(const Atom& a, const Atom& b) {
    const float ci = CovalentRadiusAngstrom(a.element);
    const float cj = CovalentRadiusAngstrom(b.element);
    float cap = ci + cj + BondDistanceConstants::kCovalentPaddingAngstrom;
    const char e1 = ElementKeyChar(a.element);
    const char e2 = ElementKeyChar(b.element);
    if (e1 == 'H' || e2 == 'H') {
        cap = std::min(cap, BondDistanceConstants::kHydrogenCapAngstrom);
    }
    if (e1 == 'O' && e2 == 'O') {
        cap = std::min(cap, BondDistanceConstants::kOxygenPairCapAngstrom);
    }
    if (e1 == 'N' && e2 == 'N') {
        cap = std::min(cap, BondDistanceConstants::kNitrogenPairCapAngstrom);
    }
    if (e1 == 'S' && e2 == 'S') {
        cap = std::max(cap, BondDistanceConstants::kSulfurPairFloorAngstrom);
        cap = std::min(cap, BondDistanceConstants::kSulfurPairCapAngstrom);
    }
    const bool metalPair = IsMetalElement(a.element) || IsMetalElement(b.element);
    if (metalPair) {
        cap = std::max(cap, BondDistanceConstants::kMetalPairFloorAngstrom);
        cap = std::min(cap, BondDistanceConstants::kMetalPairCapAngstrom);
    } else {
        cap = std::min(cap, BondDistanceConstants::kNonMetalCapAngstrom);
    }
    return cap;
}

struct ResidueKey {
    char chain = ' ';
    int seq = 0;
    char icode = ' ';

    bool operator<(const ResidueKey& o) const {
        if (chain != o.chain) {
            return chain < o.chain;
        }
        if (seq != o.seq) {
            return seq < o.seq;
        }
        const unsigned char u = static_cast<unsigned char>(icode == ' ' ? '\0' : icode);
        const unsigned char v = static_cast<unsigned char>(o.icode == ' ' ? '\0' : o.icode);
        return u < v;
    }
};

std::string CanonicalAtomName(const std::string& raw) {
    std::string s = TrimToken(raw);
    for (char& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

void AddUndirectedEdge(std::set<std::pair<int, int>>& edges, int i, int j) {
    if (i == j) {
        return;
    }
    const int a = std::min(i, j);
    const int b = std::max(i, j);
    edges.insert({a, b});
}

void MaybeBondByDistance(std::set<std::pair<int, int>>& edges, const Atom& A, int ia, const Atom& B, int ib, float dMin,
                         float dMax) {
    const float d = glm::length(A.position - B.position);
    if (d >= dMin && d <= dMax) {
        AddUndirectedEdge(edges, ia, ib);
    }
}

/// Standard peptide backbone bonds from ATOM records (N–CA, CA–C, C–N peptide).
void AddExplicitPolymerBackboneBonds(const std::vector<Atom>& atoms, std::set<std::pair<int, int>>& edges) {
    std::map<ResidueKey, std::unordered_map<std::string, int>> byResidue;
    for (int i = 0; i < static_cast<int>(atoms.size()); ++i) {
        const Atom& a = atoms[static_cast<size_t>(i)];
        if (a.hetero) {
            continue;
        }
        ResidueKey k{};
        k.chain = a.chainChar == '\0' ? ' ' : a.chainChar;
        k.seq = a.residueSeq;
        k.icode = a.insertionCode == '\0' ? ' ' : a.insertionCode;
        byResidue[k][CanonicalAtomName(a.atomName)] = i;
    }

    for (auto& resEntry : byResidue) {
        const auto& m = resEntry.second;
        const auto itN = m.find("N");
        const auto itCA = m.find("CA");
        const auto itC = m.find("C");
        if (itN != m.end() && itCA != m.end()) {
            MaybeBondByDistance(edges, atoms[static_cast<size_t>(itN->second)], itN->second, atoms[static_cast<size_t>(itCA->second)],
                                itCA->second, 0.95f, 1.75f);
        }
        if (itCA != m.end() && itC != m.end()) {
            MaybeBondByDistance(edges, atoms[static_cast<size_t>(itCA->second)], itCA->second, atoms[static_cast<size_t>(itC->second)],
                                itC->second, 1.15f, 1.85f);
        }
    }

    for (auto it = byResidue.begin(); it != byResidue.end(); ++it) {
        auto next = it;
        ++next;
        if (next == byResidue.end()) {
            break;
        }
        if (it->first.chain != next->first.chain) {
            continue;
        }
        const auto itC = it->second.find("C");
        const auto itN = next->second.find("N");
        if (itC == it->second.end() || itN == next->second.end()) {
            continue;
        }
        MaybeBondByDistance(edges, atoms[static_cast<size_t>(itC->second)], itC->second,
                            atoms[static_cast<size_t>(itN->second)], itN->second, 1.05f, 1.55f);
    }
}

struct GridKey {
    int x = 0;
    int y = 0;
    int z = 0;
    bool operator==(const GridKey& o) const { return x == o.x && y == o.y && z == o.z; }
};

struct GridKeyHash {
    std::size_t operator()(const GridKey& k) const noexcept {
        std::size_t h = 1469598103934665603ULL;
        auto mix = [&](int v) {
            h ^= static_cast<std::size_t>(static_cast<unsigned int>(v) + 0x9e3779b9u);
            h *= 1099511628211ULL;
        };
        mix(k.x);
        mix(k.y);
        mix(k.z);
        return h;
    }
};

void InferBondsSpatialHash(const std::vector<Atom>& atoms, std::set<std::pair<int, int>>& edges) {
    const int n = static_cast<int>(atoms.size());
    if (n < 2) {
        return;
    }
    constexpr float kCell = 2.85f;
    constexpr float kMinD = 0.42f;

    std::unordered_map<GridKey, std::vector<int>, GridKeyHash> grid;
    grid.reserve(static_cast<size_t>(std::max(16, n / 2)));

    auto cellOf = [&](const glm::vec3& p) -> GridKey {
        return {static_cast<int>(std::floor(p.x / kCell)), static_cast<int>(std::floor(p.y / kCell)),
                static_cast<int>(std::floor(p.z / kCell))};
    };

    for (int i = 0; i < n; ++i) {
        grid[cellOf(atoms[static_cast<size_t>(i)].position)].push_back(i);
    }

    for (int i = 0; i < n; ++i) {
        const glm::vec3& pi = atoms[static_cast<size_t>(i)].position;
        const GridKey c0 = cellOf(pi);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    const GridKey nk{c0.x + dx, c0.y + dy, c0.z + dz};
                    const auto it = grid.find(nk);
                    if (it == grid.end()) {
                        continue;
                    }
                    for (int j : it->second) {
                        if (j <= i) {
                            continue;
                        }
                        const Atom& A = atoms[static_cast<size_t>(i)];
                        const Atom& B = atoms[static_cast<size_t>(j)];
                        const float d = glm::length(A.position - B.position);
                        if (d < kMinD) {
                            continue;
                        }
                        const float dmax = MaxAllowedBondDistance(A, B);
                        if (d > dmax) {
                            continue;
                        }
                        AddUndirectedEdge(edges, i, j);
                    }
                }
            }
        }
    }
}

struct CaNode {
    int atomIndex = 0;
    char chain = ' ';
    int resSeq = 0;
    char icode = ' ';
    glm::vec3 pos{0.0f};
};

bool CaNodeLess(const CaNode& a, const CaNode& b) {
    if (a.chain != b.chain) {
        return a.chain < b.chain;
    }
    if (a.resSeq != b.resSeq) {
        return a.resSeq < b.resSeq;
    }
    const unsigned char ua = static_cast<unsigned char>(a.icode == ' ' ? '\0' : a.icode);
    const unsigned char ub = static_cast<unsigned char>(b.icode == ' ' ? '\0' : b.icode);
    if (ua != ub) {
        return ua < ub;
    }
    return a.atomIndex < b.atomIndex;
}

struct AtomSiteKey {
    bool hetero = false;
    char chain = ' ';
    int resSeq = 0;
    char icode = ' ';
    std::string resName;
    std::string atomName;

    bool operator<(const AtomSiteKey& o) const {
        if (hetero != o.hetero) {
            return hetero < o.hetero;
        }
        if (chain != o.chain) {
            return chain < o.chain;
        }
        if (resSeq != o.resSeq) {
            return resSeq < o.resSeq;
        }
        const unsigned char ui = static_cast<unsigned char>(icode == ' ' ? '\0' : icode);
        const unsigned char uo = static_cast<unsigned char>(o.icode == ' ' ? '\0' : o.icode);
        if (ui != uo) {
            return ui < uo;
        }
        if (resName != o.resName) {
            return resName < o.resName;
        }
        return atomName < o.atomName;
    }
};

AtomSiteKey MakeSiteKey(const Atom& a) {
    AtomSiteKey k;
    k.hetero = a.hetero;
    k.chain = (a.chainChar == '\0' || a.chainChar == ' ') ? ' ' : a.chainChar;
    k.resSeq = a.residueSeq;
    k.icode = (a.insertionCode == '\0' || a.insertionCode == ' ') ? ' ' : a.insertionCode;
    k.resName = a.residueName;
    k.atomName = CanonicalAtomName(a.atomName);
    return k;
}

int AltLocRank(char c) {
    if (c == ' ' || c == '.' || c == '\0') {
        return 0;
    }
    if (c == 'A') {
        return 1;
    }
    if (c >= 'B' && c <= 'Z') {
        return 2 + (c - 'B');
    }
    return 64;
}

void ApplyAltLocChoice(std::vector<Atom>& raw, Protein& protein) {
    std::map<AtomSiteKey, size_t> winner;
    for (size_t i = 0; i < raw.size(); ++i) {
        const AtomSiteKey k = MakeSiteKey(raw[i]);
        const auto it = winner.find(k);
        if (it == winner.end()) {
            winner[k] = i;
        } else {
            const size_t j = it->second;
            if (AltLocRank(raw[i].altLoc) < AltLocRank(raw[j].altLoc)) {
                winner[k] = i;
            }
        }
    }
    std::unordered_set<size_t> keep;
    keep.reserve(winner.size());
    for (const auto& p : winner) {
        keep.insert(p.second);
    }
    std::vector<Atom> out;
    out.reserve(keep.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (keep.count(i) != 0U) {
            out.push_back(std::move(raw[i]));
        }
    }
    protein.SetAtoms(std::move(out));
}

SSType SsFromAtomTable(const std::vector<std::uint8_t>& atomSs, int atomIndex) {
    if (atomIndex < 0 || static_cast<size_t>(atomIndex) >= atomSs.size()) {
        return SSType::Coil;
    }
    return static_cast<SSType>(atomSs[static_cast<size_t>(atomIndex)]);
}

struct CaTubeBuild {
    std::vector<std::pair<int, int>> segs;
    std::vector<std::uint8_t> segSs;
};

CaTubeBuild BuildCaTubeSegmentsWithSs(const std::vector<Atom>& atoms, const std::vector<std::uint8_t>& atomSs) {
    std::vector<CaNode> nodes;
    nodes.reserve(atoms.size() / 4);
    for (int i = 0; i < static_cast<int>(atoms.size()); ++i) {
        const Atom& a = atoms[static_cast<size_t>(i)];
        if (a.hetero) {
            continue;
        }
        if (CanonicalAtomName(a.atomName) != "CA") {
            continue;
        }
        CaNode n{};
        n.atomIndex = i;
        n.chain = a.chainChar == '\0' ? ' ' : a.chainChar;
        n.resSeq = a.residueSeq;
        n.icode = a.insertionCode == '\0' ? ' ' : a.insertionCode;
        n.pos = a.position;
        nodes.push_back(n);
    }
    std::sort(nodes.begin(), nodes.end(), CaNodeLess);

    CaTubeBuild out;
    out.segs.reserve(nodes.size());
    out.segSs.reserve(nodes.size());
    for (size_t k = 0; k + 1 < nodes.size(); ++k) {
        const CaNode& u = nodes[k];
        const CaNode& v = nodes[k + 1];
        if (u.chain != v.chain) {
            continue;
        }
        const float d = glm::length(u.pos - v.pos);
        if (d < 2.35f || d > 4.85f) {
            continue;
        }
        const int seqGap = std::abs(v.resSeq - u.resSeq);
        if (seqGap > 4) {
            continue;
        }
        out.segs.push_back({u.atomIndex, v.atomIndex});
        const SSType merged = SsMerge(SsFromAtomTable(atomSs, u.atomIndex), SsFromAtomTable(atomSs, v.atomIndex));
        out.segSs.push_back(static_cast<std::uint8_t>(merged));
    }
    return out;
}

bool TryParseHelixRange(const std::string& line, char& chainA, int& seqA, char& chainB, int& seqB) {
    if (line.size() < 36 || line.compare(0, 5, "HELIX") != 0) {
        return false;
    }
    chainA = line.size() > 19 ? line[19] : ' ';
    seqA = ParseIntField(line, 21, 4);
    chainB = line.size() > 31 ? line[31] : chainA;
    seqB = ParseIntField(line, 32, 4);
    return seqA != 0 && seqB != 0;
}

bool TryParseSheetRange(const std::string& line, char& chainA, int& seqA, char& chainB, int& seqB) {
    if (line.size() < 37 || line.compare(0, 5, "SHEET") != 0) {
        return false;
    }
    chainA = line.size() > 21 ? line[21] : ' ';
    seqA = ParseIntField(line, 22, 4);
    chainB = line.size() > 32 ? line[32] : chainA;
    seqB = ParseIntField(line, 33, 4);
    return seqA != 0 && seqB != 0;
}

void ApplySsRange(std::vector<std::uint8_t>& atomSs, const std::vector<Atom>& atoms, char chain, int seqLo, int seqHi,
                  SSType assign) {
    if (seqLo > seqHi) {
        std::swap(seqLo, seqHi);
    }
    for (size_t i = 0; i < atoms.size(); ++i) {
        const Atom& a = atoms[i];
        const char c = a.chainChar == '\0' ? ' ' : a.chainChar;
        if (c != chain) {
            continue;
        }
        if (a.residueSeq < seqLo || a.residueSeq > seqHi) {
            continue;
        }
        atomSs[i] = static_cast<std::uint8_t>(SsMerge(static_cast<SSType>(atomSs[i]), assign));
    }
}

struct ResidueBackbone {
    char chain = ' ';
    int seq = 0;
    char icode = ' ';
    int n = -1;
    int ca = -1;
    int c = -1;
    int o = -1;
};

bool ResidueBackboneLess(const ResidueBackbone& a, const ResidueBackbone& b) {
    if (a.chain != b.chain) {
        return a.chain < b.chain;
    }
    if (a.seq != b.seq) {
        return a.seq < b.seq;
    }
    const unsigned char ua = static_cast<unsigned char>(a.icode == ' ' ? '\0' : a.icode);
    const unsigned char ub = static_cast<unsigned char>(b.icode == ' ' ? '\0' : b.icode);
    return ua < ub;
}

std::vector<ResidueBackbone> BuildBackboneResidues(const std::vector<Atom>& atoms) {
    std::map<ResidueKey, ResidueBackbone> byRes;
    for (int i = 0; i < static_cast<int>(atoms.size()); ++i) {
        const Atom& a = atoms[static_cast<size_t>(i)];
        if (a.hetero) {
            continue;
        }
        ResidueKey rk{};
        rk.chain = a.chainChar == '\0' ? ' ' : a.chainChar;
        rk.seq = a.residueSeq;
        rk.icode = a.insertionCode == '\0' ? ' ' : a.insertionCode;
        ResidueBackbone& bb = byRes[rk];
        bb.chain = rk.chain;
        bb.seq = rk.seq;
        bb.icode = rk.icode;
        const std::string an = CanonicalAtomName(a.atomName);
        if (an == "N") {
            bb.n = i;
        } else if (an == "CA") {
            bb.ca = i;
        } else if (an == "C") {
            bb.c = i;
        } else if (an == "O") {
            bb.o = i;
        }
    }
    std::vector<ResidueBackbone> out;
    out.reserve(byRes.size());
    for (const auto& kv : byRes) {
        out.push_back(kv.second);
    }
    std::sort(out.begin(), out.end(), ResidueBackboneLess);
    return out;
}

float DsspHydrogenBondEnergy(const glm::vec3& nPos, const glm::vec3& hPos, const glm::vec3& cPos, const glm::vec3& oPos) {
    const float rON = glm::distance(oPos, nPos);
    const float rCH = glm::distance(cPos, hPos);
    const float rOH = glm::distance(oPos, hPos);
    const float rCN = glm::distance(cPos, nPos);
    const float eps = 1.0e-4f;
    if (rON < eps || rCH < eps || rOH < eps || rCN < eps) {
        return 0.0f;
    }
    return 27.888f * ((1.0f / rON) + (1.0f / rCH) - (1.0f / rOH) - (1.0f / rCN));
}

std::vector<std::uint8_t> AssignSecondaryStructureDsspLike(const std::vector<Atom>& atoms) {
    std::vector<std::uint8_t> atomSs(atoms.size(), static_cast<std::uint8_t>(SSType::Coil));
    std::vector<ResidueBackbone> residues = BuildBackboneResidues(atoms);
    if (residues.size() < 4) {
        return atomSs;
    }

    std::vector<glm::vec3> hPos(residues.size(), glm::vec3(0.0f));
    std::vector<bool> haveH(residues.size(), false);
    for (size_t i = 1; i < residues.size(); ++i) {
        if (residues[i].chain != residues[i - 1].chain) {
            continue;
        }
        const ResidueBackbone& cur = residues[i];
        const ResidueBackbone& prev = residues[i - 1];
        if (cur.n < 0 || prev.c < 0 || prev.o < 0) {
            continue;
        }
        const glm::vec3 n = atoms[static_cast<size_t>(cur.n)].position;
        const glm::vec3 c = atoms[static_cast<size_t>(prev.c)].position;
        const glm::vec3 o = atoms[static_cast<size_t>(prev.o)].position;
        glm::vec3 v1 = n - c;
        glm::vec3 v2 = n - o;
        if (glm::length(v1) < 1.0e-5f || glm::length(v2) < 1.0e-5f) {
            continue;
        }
        v1 = glm::normalize(v1);
        v2 = glm::normalize(v2);
        glm::vec3 hDir = v1 + v2;
        if (glm::length(hDir) < 1.0e-5f) {
            continue;
        }
        hDir = glm::normalize(hDir);
        hPos[i] = n + hDir;
        haveH[i] = true;
    }

    std::vector<std::vector<bool>> hb(residues.size(), std::vector<bool>(residues.size(), false));
    for (size_t i = 0; i < residues.size(); ++i) {
        if (!haveH[i] || residues[i].n < 0) {
            continue;
        }
        for (size_t j = 0; j < residues.size(); ++j) {
            if (i == j || residues[j].c < 0 || residues[j].o < 0) {
                continue;
            }
            if (residues[i].chain != residues[j].chain) {
                continue;
            }
            if (std::abs(static_cast<int>(i) - static_cast<int>(j)) <= 1) {
                continue;
            }
            const glm::vec3 n = atoms[static_cast<size_t>(residues[i].n)].position;
            const glm::vec3 h = hPos[i];
            const glm::vec3 c = atoms[static_cast<size_t>(residues[j].c)].position;
            const glm::vec3 o = atoms[static_cast<size_t>(residues[j].o)].position;
            const float e = DsspHydrogenBondEnergy(n, h, c, o);
            if (e < -0.5f) {
                hb[i][j] = true;
            }
        }
    }

    std::vector<SSType> resSs(residues.size(), SSType::Coil);
    auto markHelixTurn = [&](int turn) {
        for (size_t i = 0; i + static_cast<size_t>(turn) < residues.size(); ++i) {
            const size_t j = i + static_cast<size_t>(turn);
            if (residues[i].chain != residues[j].chain) {
                continue;
            }
            if (hb[i][j]) {
                for (size_t k = i; k <= j; ++k) {
                    resSs[k] = SsMerge(resSs[k], SSType::Helix);
                }
            }
        }
    };
    markHelixTurn(3);
    markHelixTurn(4);
    markHelixTurn(5);

    for (size_t i = 0; i < residues.size(); ++i) {
        for (size_t j = 0; j < residues.size(); ++j) {
            if (!hb[i][j]) {
                continue;
            }
            if (residues[i].chain != residues[j].chain) {
                continue;
            }
            if (std::abs(static_cast<int>(i) - static_cast<int>(j)) < 4) {
                continue;
            }
            resSs[i] = SsMerge(resSs[i], SSType::Strand);
            resSs[j] = SsMerge(resSs[j], SSType::Strand);
            if (i + 1 < residues.size() && residues[i + 1].chain == residues[i].chain) {
                resSs[i + 1] = SsMerge(resSs[i + 1], SSType::Strand);
            }
            if (j + 1 < residues.size() && residues[j + 1].chain == residues[j].chain) {
                resSs[j + 1] = SsMerge(resSs[j + 1], SSType::Strand);
            }
        }
    }

    for (size_t ri = 0; ri < residues.size(); ++ri) {
        const ResidueBackbone& r = residues[ri];
        if (r.ca >= 0) {
            atomSs[static_cast<size_t>(r.ca)] = static_cast<std::uint8_t>(resSs[ri]);
        }
        if (r.n >= 0) {
            atomSs[static_cast<size_t>(r.n)] = static_cast<std::uint8_t>(SsMerge(static_cast<SSType>(atomSs[static_cast<size_t>(r.n)]), resSs[ri]));
        }
        if (r.c >= 0) {
            atomSs[static_cast<size_t>(r.c)] = static_cast<std::uint8_t>(SsMerge(static_cast<SSType>(atomSs[static_cast<size_t>(r.c)]), resSs[ri]));
        }
        if (r.o >= 0) {
            atomSs[static_cast<size_t>(r.o)] = static_cast<std::uint8_t>(SsMerge(static_cast<SSType>(atomSs[static_cast<size_t>(r.o)]), resSs[ri]));
        }
    }
    return atomSs;
}

struct BridgePair {
    int i = 0;
    int j = 0;
    bool parallel = true;
};

bool InRange(const std::vector<std::vector<bool>>& hb, int i, int j) {
    if (i < 0 || j < 0) {
        return false;
    }
    if (i >= static_cast<int>(hb.size()) || j >= static_cast<int>(hb.size())) {
        return false;
    }
    return hb[static_cast<size_t>(i)][static_cast<size_t>(j)];
}

std::vector<char> AssignSecondaryStructureStrictDssp(const std::vector<Atom>& atoms) {
    std::vector<ResidueBackbone> residues = BuildBackboneResidues(atoms);
    std::vector<char> labels(residues.size(), 'C');
    if (residues.size() < 4) {
        return labels;
    }

    std::vector<glm::vec3> hPos(residues.size(), glm::vec3(0.0f));
    std::vector<bool> haveH(residues.size(), false);
    for (size_t i = 1; i < residues.size(); ++i) {
        if (residues[i].chain != residues[i - 1].chain) {
            continue;
        }
        const ResidueBackbone& cur = residues[i];
        const ResidueBackbone& prev = residues[i - 1];
        if (cur.n < 0 || prev.c < 0 || prev.o < 0) {
            continue;
        }
        const glm::vec3 n = atoms[static_cast<size_t>(cur.n)].position;
        const glm::vec3 c = atoms[static_cast<size_t>(prev.c)].position;
        const glm::vec3 o = atoms[static_cast<size_t>(prev.o)].position;
        glm::vec3 v1 = n - c;
        glm::vec3 v2 = n - o;
        if (glm::length(v1) < 1.0e-5f || glm::length(v2) < 1.0e-5f) {
            continue;
        }
        v1 = glm::normalize(v1);
        v2 = glm::normalize(v2);
        glm::vec3 hDir = v1 + v2;
        if (glm::length(hDir) < 1.0e-5f) {
            continue;
        }
        hDir = glm::normalize(hDir);
        hPos[i] = n + hDir;
        haveH[i] = true;
    }

    std::vector<std::vector<bool>> hb(residues.size(), std::vector<bool>(residues.size(), false));
    for (size_t i = 0; i < residues.size(); ++i) {
        if (!haveH[i] || residues[i].n < 0) {
            continue;
        }
        for (size_t j = 0; j < residues.size(); ++j) {
            if (i == j || residues[j].c < 0 || residues[j].o < 0) {
                continue;
            }
            if (residues[i].chain != residues[j].chain) {
                continue;
            }
            if (std::abs(static_cast<int>(i) - static_cast<int>(j)) <= 1) {
                continue;
            }
            const glm::vec3 n = atoms[static_cast<size_t>(residues[i].n)].position;
            const glm::vec3 h = hPos[i];
            const glm::vec3 c = atoms[static_cast<size_t>(residues[j].c)].position;
            const glm::vec3 o = atoms[static_cast<size_t>(residues[j].o)].position;
            const float e = DsspHydrogenBondEnergy(n, h, c, o);
            if (e < -0.5f) {
                hb[i][j] = true;
            }
        }
    }

    std::vector<bool> turn3(residues.size(), false);
    std::vector<bool> turn4(residues.size(), false);
    std::vector<bool> turn5(residues.size(), false);
    for (size_t i = 0; i + 3 < residues.size(); ++i) {
        if (residues[i].chain == residues[i + 3].chain && hb[i][i + 3]) {
            turn3[i] = true;
        }
    }
    for (size_t i = 0; i + 4 < residues.size(); ++i) {
        if (residues[i].chain == residues[i + 4].chain && hb[i][i + 4]) {
            turn4[i] = true;
        }
    }
    for (size_t i = 0; i + 5 < residues.size(); ++i) {
        if (residues[i].chain == residues[i + 5].chain && hb[i][i + 5]) {
            turn5[i] = true;
        }
    }

    for (size_t i = 0; i < residues.size(); ++i) {
        if (turn3[i]) {
            for (size_t k = i + 1; k <= i + 2 && k < labels.size(); ++k) {
                labels[k] = (labels[k] == 'C') ? 'T' : labels[k];
            }
        }
        if (turn4[i]) {
            for (size_t k = i + 1; k <= i + 3 && k < labels.size(); ++k) {
                labels[k] = (labels[k] == 'C') ? 'T' : labels[k];
            }
        }
        if (turn5[i]) {
            for (size_t k = i + 1; k <= i + 4 && k < labels.size(); ++k) {
                labels[k] = (labels[k] == 'C') ? 'T' : labels[k];
            }
        }
    }

    for (size_t i = 0; i + 5 < residues.size(); ++i) {
        if (turn5[i]) {
            for (size_t k = i + 1; k <= i + 4 && k < labels.size(); ++k) {
                labels[k] = 'I';
            }
        }
    }
    for (size_t i = 0; i + 4 < residues.size(); ++i) {
        if (turn4[i]) {
            for (size_t k = i + 1; k <= i + 3 && k < labels.size(); ++k) {
                labels[k] = 'H';
            }
        }
    }
    for (size_t i = 0; i + 3 < residues.size(); ++i) {
        if (turn3[i]) {
            for (size_t k = i + 1; k <= i + 2 && k < labels.size(); ++k) {
                if (labels[k] == 'C' || labels[k] == 'T') {
                    labels[k] = 'G';
                }
            }
        }
    }

    std::vector<BridgePair> bridges;
    for (int i = 1; i + 1 < static_cast<int>(residues.size()); ++i) {
        for (int j = i + 2; j + 1 < static_cast<int>(residues.size()); ++j) {
            if (residues[static_cast<size_t>(i)].chain != residues[static_cast<size_t>(j)].chain) {
                continue;
            }
            const bool anti = (InRange(hb, i, j) && InRange(hb, j, i)) ||
                              (InRange(hb, i - 1, j + 1) && InRange(hb, j - 1, i + 1));
            const bool para = (InRange(hb, i - 1, j) && InRange(hb, j, i + 1)) ||
                              (InRange(hb, j - 1, i) && InRange(hb, i, j + 1));
            if (!anti && !para) {
                continue;
            }
            bridges.push_back({i, j, para});
            if (labels[static_cast<size_t>(i)] == 'C') {
                labels[static_cast<size_t>(i)] = 'B';
            }
            if (labels[static_cast<size_t>(j)] == 'C') {
                labels[static_cast<size_t>(j)] = 'B';
            }
        }
    }

    std::sort(bridges.begin(), bridges.end(), [](const BridgePair& a, const BridgePair& b) {
        if (a.parallel != b.parallel) {
            return a.parallel < b.parallel;
        }
        if (a.i != b.i) {
            return a.i < b.i;
        }
        return a.j < b.j;
    });

    std::vector<bool> promotedE(labels.size(), false);
    for (size_t bi = 0; bi < bridges.size(); ++bi) {
        std::vector<BridgePair> ladder;
        ladder.push_back(bridges[bi]);
        int prevI = bridges[bi].i;
        int prevJ = bridges[bi].j;
        for (size_t bj = bi + 1; bj < bridges.size(); ++bj) {
            if (bridges[bj].parallel != bridges[bi].parallel) {
                continue;
            }
            const int di = bridges[bj].i - prevI;
            const int dj = bridges[bj].parallel ? (bridges[bj].j - prevJ) : (prevJ - bridges[bj].j);
            if (di == 1 && dj == 1) {
                ladder.push_back(bridges[bj]);
                prevI = bridges[bj].i;
                prevJ = bridges[bj].j;
            }
        }
        if (ladder.size() >= 2) {
            for (const BridgePair& b : ladder) {
                promotedE[static_cast<size_t>(b.i)] = true;
                promotedE[static_cast<size_t>(b.j)] = true;
            }
        }
    }

    for (size_t i = 0; i < labels.size(); ++i) {
        if (promotedE[i]) {
            labels[i] = 'E';
        }
    }
    return labels;
}

std::vector<std::uint8_t> MapResidueLabelsToAtomSs(const std::vector<Atom>& atoms, const std::vector<ResidueBackbone>& residues,
                                                   const std::vector<char>& labels) {
    std::vector<std::uint8_t> atomSs(atoms.size(), static_cast<std::uint8_t>(SSType::Coil));
    for (size_t ri = 0; ri < residues.size() && ri < labels.size(); ++ri) {
        SSType s = SSType::Coil;
        const char lab = labels[ri];
        if (lab == 'H' || lab == 'G' || lab == 'I') {
            s = SSType::Helix;
        } else if (lab == 'E' || lab == 'B') {
            s = SSType::Strand;
        }
        const ResidueBackbone& r = residues[ri];
        if (r.ca >= 0) {
            atomSs[static_cast<size_t>(r.ca)] = static_cast<std::uint8_t>(s);
        }
        if (r.n >= 0) {
            atomSs[static_cast<size_t>(r.n)] =
                static_cast<std::uint8_t>(SsMerge(static_cast<SSType>(atomSs[static_cast<size_t>(r.n)]), s));
        }
        if (r.c >= 0) {
            atomSs[static_cast<size_t>(r.c)] =
                static_cast<std::uint8_t>(SsMerge(static_cast<SSType>(atomSs[static_cast<size_t>(r.c)]), s));
        }
        if (r.o >= 0) {
            atomSs[static_cast<size_t>(r.o)] =
                static_cast<std::uint8_t>(SsMerge(static_cast<SSType>(atomSs[static_cast<size_t>(r.o)]), s));
        }
    }
    return atomSs;
}

void FinishProteinModel(Protein& protein, const std::set<std::pair<int, int>>& serialEdges,
                        const std::vector<std::string>& helixLines, const std::vector<std::string>& sheetLines,
                        const std::vector<SecondaryStructureRange>& mmcifRanges) {
    const auto& atoms = protein.GetAtoms();
    std::unordered_map<int, int> serialToIndex;
    serialToIndex.reserve(atoms.size() * 2);
    for (size_t i = 0; i < atoms.size(); ++i) {
        if (atoms[i].serial > 0) {
            serialToIndex[atoms[i].serial] = static_cast<int>(i);
        }
    }

    std::set<std::pair<int, int>> indexEdges;
    for (const auto& e : serialEdges) {
        const auto it0 = serialToIndex.find(e.first);
        const auto it1 = serialToIndex.find(e.second);
        if (it0 != serialToIndex.end() && it1 != serialToIndex.end()) {
            const int i0 = it0->second;
            const int i1 = it1->second;
            if (i0 != i1) {
                const int a = std::min(i0, i1);
                const int b = std::max(i0, i1);
                indexEdges.insert({a, b});
            }
        }
    }

    AddExplicitPolymerBackboneBonds(atoms, indexEdges);
    InferBondsSpatialHash(atoms, indexEdges);

    std::vector<std::pair<int, int>> bonds(indexEdges.begin(), indexEdges.end());
    protein.SetBonds(std::move(bonds));

    std::vector<std::uint8_t> atomSs;
    if (g_StrictDsspMode) {
        const std::vector<ResidueBackbone> residues = BuildBackboneResidues(atoms);
        const std::vector<char> labels = AssignSecondaryStructureStrictDssp(atoms);
        atomSs = MapResidueLabelsToAtomSs(atoms, residues, labels);
    } else {
        atomSs = AssignSecondaryStructureDsspLike(atoms);
    }
    for (const std::string& hl : helixLines) {
        char ca = ' ';
        char cb = ' ';
        int sa = 0;
        int sb = 0;
        if (!TryParseHelixRange(hl, ca, sa, cb, sb)) {
            continue;
        }
        if (ca == cb) {
            ApplySsRange(atomSs, atoms, ca, sa, sb, SSType::Helix);
        }
    }
    for (const std::string& sl : sheetLines) {
        char ca = ' ';
        char cb = ' ';
        int sa = 0;
        int sb = 0;
        if (!TryParseSheetRange(sl, ca, sa, cb, sb)) {
            continue;
        }
        if (ca == cb) {
            ApplySsRange(atomSs, atoms, ca, sa, sb, SSType::Strand);
        }
    }
    for (const SecondaryStructureRange& r : mmcifRanges) {
        if (r.type == SSType::Coil) {
            continue;
        }
        ApplySsRange(atomSs, atoms, r.chain, r.startSeq, r.endSeq, r.type);
    }

    CaTubeBuild tube = BuildCaTubeSegmentsWithSs(atoms, atomSs);
    protein.SetAtomSecondaryStructure(std::move(atomSs));
    protein.SetCaTubeSegments(std::move(tube.segs));
    protein.SetCaTubeSegmentSs(std::move(tube.segSs));
}

} // namespace detail

void PDBParser::SetStrictDsspMode(bool enabled) {
    detail::g_StrictDsspMode = enabled;
}

Protein PDBParser::ParsePDBFile(const std::string& filepath) {
    Protein protein;
    protein.SetName(filepath);

    std::ifstream file(filepath);
    if (!file) {
        throw std::runtime_error("Could not open PDB file: " + filepath);
    }

    bool seenModelKeyword = false;
    bool inFirstModel = false;
    bool pastFirstModelBlock = false;

    std::set<std::pair<int, int>> serialEdges;
    std::vector<Atom> rawAtoms;
    rawAtoms.reserve(4096);
    std::vector<std::string> helixLines;
    std::vector<std::string> sheetLines;

    std::string line;
    while (std::getline(file, line)) {
        if (line.size() >= 6 && line.compare(0, 6, "CONECT") == 0) {
            std::vector<int> serials;
            for (size_t off = 6; off + 5 <= line.size(); off += 5) {
                const int v = detail::ParseIntField(line, off, 5);
                if (v == 0) {
                    break;
                }
                serials.push_back(v);
            }
            if (serials.size() >= 2) {
                const int s0 = serials[0];
                for (size_t k = 1; k < serials.size(); ++k) {
                    const int s1 = serials[k];
                    if (s1 == 0) {
                        continue;
                    }
                    const int a = std::min(s0, s1);
                    const int b = std::max(s0, s1);
                    serialEdges.insert({a, b});
                }
            }
            continue;
        }

        if (line.size() >= 5 && line.compare(0, 5, "MODEL") == 0) {
            seenModelKeyword = true;
            int modelId = 1;
            if (line.size() >= 14) {
                modelId = std::max(1, detail::ParseIntField(line, 10, 4));
            }
            inFirstModel = (modelId == 1);
            continue;
        }

        if (line.size() >= 6 && line.compare(0, 6, "ENDMDL") == 0) {
            if (seenModelKeyword && inFirstModel) {
                pastFirstModelBlock = true;
            }
            inFirstModel = false;
            continue;
        }

        if (line.size() >= 5 && line.compare(0, 5, "HELIX") == 0) {
            helixLines.push_back(line);
            continue;
        }
        if (line.size() >= 5 && line.compare(0, 5, "SHEET") == 0) {
            sheetLines.push_back(line);
            continue;
        }

        if (line.size() < 54) {
            continue;
        }

        const bool isAtom = (line.size() >= 4 && line.compare(0, 4, "ATOM") == 0);
        const bool isHetatm = (line.size() >= 6 && line.compare(0, 6, "HETATM") == 0);
        if (!isAtom && !isHetatm) {
            continue;
        }

        if (pastFirstModelBlock) {
            continue;
        }
        if (seenModelKeyword && !inFirstModel) {
            continue;
        }

        Atom atom{};
        const int serial = detail::ParseIntField(line, 6, 5);
        atom.serial = serial > 0 ? serial : -1;

        try {
            atom.position.x = std::stof(line.substr(30, 8));
            atom.position.y = std::stof(line.substr(38, 8));
            atom.position.z = std::stof(line.substr(46, 8));
        } catch (...) {
            continue;
        }

        std::string elem;
        if (line.size() >= 78) {
            elem = detail::TrimToken(line.substr(76, 2));
        }
        if (elem.empty() && line.size() >= 16) {
            elem = detail::TrimToken(line.substr(12, 2));
        }
        atom.element = elem.empty() ? "X" : elem;
        atom.radius = detail::VdwRadiusForElement(atom.element);

        if (line.size() >= 16) {
            atom.atomName = detail::TrimToken(line.substr(12, 4));
        }
        if (line.size() >= 20) {
            atom.residueName = detail::TrimToken(line.substr(17, 3));
        }
        atom.residueSeq = detail::ParseIntField(line, 22, 4);
        if (line.size() > 21) {
            atom.chainChar = line[21];
        } else {
            atom.chainChar = ' ';
        }
        if (line.size() > 26) {
            atom.insertionCode = line[26];
        } else {
            atom.insertionCode = ' ';
        }
        atom.hetero = isHetatm;
        atom.chainId = static_cast<int>(static_cast<unsigned char>(atom.chainChar));

        if (line.size() > 16) {
            const char a = line[16];
            atom.altLoc = (a == ' ' || a == '.' || a == '\t') ? ' ' : a;
        } else {
            atom.altLoc = ' ';
        }

        rawAtoms.push_back(std::move(atom));
    }

    detail::ApplyAltLocChoice(rawAtoms, protein);
    PDBParser::FinalizeStructure(protein, serialEdges, helixLines, sheetLines, {});

    return protein;
}

void PDBParser::FinalizeStructure(Protein& protein, const std::set<std::pair<int, int>>& serialEdges,
                                  const std::vector<std::string>& helixLines, const std::vector<std::string>& sheetLines,
                                  const std::vector<SecondaryStructureRange>& mmcifRanges) {
    detail::FinishProteinModel(protein, serialEdges, helixLines, sheetLines, mmcifRanges);
}

void PDBParser::ApplyAtomSiteAltLoc(std::vector<Atom>& rawAtoms, Protein& protein) {
    detail::ApplyAltLocChoice(rawAtoms, protein);
}
