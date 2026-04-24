#include "dxcc/DxccDatabase.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

namespace TwoPLogger {

// ── file-local helpers ────────────────────────────────────────────────────────

namespace {

std::string toUpper(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s)
{
    auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// Strip override markers from a raw alias token such as "UA9F(17)[30]".
// Returns the bare prefix/callsign; fills cqOverride and ituOverride (0 = absent).
std::string parseOverrides(const std::string& raw, int& cqOverride, int& ituOverride)
{
    cqOverride  = 0;
    ituOverride = 0;
    std::string base;
    base.reserve(raw.size());

    for (std::size_t i = 0; i < raw.size(); ) {
        char c = raw[i];
        if (c == '(') {
            auto end = raw.find(')', i);
            if (end != std::string::npos) {
                cqOverride = std::stoi(raw.substr(i + 1, end - i - 1));
                i = end + 1;
            } else { ++i; }
        } else if (c == '[') {
            auto end = raw.find(']', i);
            if (end != std::string::npos) {
                ituOverride = std::stoi(raw.substr(i + 1, end - i - 1));
                i = end + 1;
            } else { ++i; }
        } else if (c == '<') {
            // UTC offset override — ignored per-alias (not in DxccEntity model)
            auto end = raw.find('>', i);
            i = (end != std::string::npos) ? end + 1 : i + 1;
        } else if (c == '{') {
            // continent override — ignored per-alias
            auto end = raw.find('}', i);
            i = (end != std::string::npos) ? end + 1 : i + 1;
        } else {
            base += c;
            ++i;
        }
    }
    return base;
}

// Parse an entity header line of the form:
//   Name: CQ: ITU: Cont: Lat: Lon: UTC: Prefix:
// cty.dat longitude convention: positive = West. We negate to store positive = East.
// Returns false if the line doesn't look like a header (< 8 colon-separated fields).
bool parseHeaderLine(const std::string& line, DxccEntity& entity, bool& isDeleted)
{
    isDeleted = false;
    std::vector<std::string> fields;
    fields.reserve(9);
    std::string field;
    for (char c : line) {
        if (c == ':') {
            fields.push_back(trim(field));
            field.clear();
        } else {
            field += c;
        }
    }
    if (fields.size() < 8) return false;

    try {
        entity.name      = fields[0];
        entity.cqZone    = std::stoi(fields[1]);
        entity.ituZone   = std::stoi(fields[2]);
        entity.continent = fields[3];
        entity.lat       = std::stod(fields[4]);
        entity.lon       = -std::stod(fields[5]);  // negate: cty.dat is positive=West
        entity.utcOffset = std::stof(fields[6]);

        std::string prefix = fields[7];
        if (!prefix.empty() && prefix[0] == '*') {
            isDeleted = true;
            prefix    = prefix.substr(1);
        }
        entity.prefix     = toUpper(trim(prefix));
        entity.isDeleted  = isDeleted;
    } catch (...) {
        return false;
    }

    return !entity.name.empty() && !entity.prefix.empty();
}

} // anonymous namespace

// ── DxccDatabase public ───────────────────────────────────────────────────────

bool DxccDatabase::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    m_entities.clear();
    m_prefixMap.clear();
    m_exactMap.clear();
    m_loaded = false;

    std::string line;
    bool haveEntity = false;

    // Process one comma/semicolon-separated token from an alias block.
    // entityIdx is the index of the entity this alias belongs to.
    auto addToken = [&](const std::string& raw, std::size_t entityIdx) {
        if (raw.empty()) return;

        bool isExact = (raw[0] == '=');
        std::string token = isExact ? raw.substr(1) : raw;

        int cqOvr = 0, ituOvr = 0;
        std::string base = toUpper(parseOverrides(token, cqOvr, ituOvr));
        if (base.empty()) return;

        PrefixEntry entry;
        entry.entityIdx       = entityIdx;
        entry.cqZoneOverride  = cqOvr;
        entry.ituZoneOverride = ituOvr;

        if (isExact)
            m_exactMap[base] = entry;
        else
            m_prefixMap[base] = entry;
    };

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        bool isAlias = (line[0] == ' ' || line[0] == '\t');

        if (!isAlias) {
            DxccEntity entity;
            bool isDeleted = false;
            if (parseHeaderLine(line, entity, isDeleted)) {
                m_entities.push_back(entity);
                haveEntity = true;
            }
        } else if (haveEntity) {
            std::size_t entityIdx = m_entities.size() - 1;
            std::string tok;
            for (char c : trim(line)) {
                if (c == ',' || c == ';') {
                    addToken(trim(tok), entityIdx);
                    tok.clear();
                    if (c == ';') break;
                } else {
                    tok += c;
                }
            }
            // No partial token at end — tokens are always complete within a line.
        }
    }

    m_loaded = !m_entities.empty();
    return m_loaded;
}

// ── DxccDatabase private ──────────────────────────────────────────────────────

DxccEntity DxccDatabase::applyOverrides(const PrefixEntry& e) const
{
    DxccEntity result = m_entities[e.entityIdx];
    if (e.cqZoneOverride  != 0) result.cqZone  = e.cqZoneOverride;
    if (e.ituZoneOverride != 0) result.ituZone = e.ituZoneOverride;
    return result;
}

// Returns the best (longest) prefix match and its matched prefix length.
std::pair<const DxccDatabase::PrefixEntry*, int>
DxccDatabase::longestPrefixMatchWithLen(const std::string& call) const
{
    for (int len = static_cast<int>(call.size()); len >= 1; --len) {
        auto it = m_prefixMap.find(call.substr(0, len));
        if (it != m_prefixMap.end())
            return {&it->second, len};
    }
    return {nullptr, 0};
}

// ── DxccDatabase::lookup ──────────────────────────────────────────────────────

DxccEntity DxccDatabase::lookup(const std::string& callsign) const
{
    if (!m_loaded || callsign.empty()) return DxccEntity::invalid();

    const std::string call = toUpper(callsign);

    // 1. Exact callsign match (highest priority — e.g. =VK9BSA in cty.dat).
    {
        auto it = m_exactMap.find(call);
        if (it != m_exactMap.end()) return applyOverrides(it->second);
    }

    // 2. If the callsign contains '/', try the full call, lhs, and rhs as
    //    prefix candidates.  The candidate whose matched prefix is longest wins.
    //    This correctly handles both "W6RGG/VK9X" (rhs wins → Christmas Is.)
    //    and "K1XX/P" (lhs wins → USA, since "P" matches nothing useful).
    auto slashPos = call.find('/');
    if (slashPos != std::string::npos) {
        const std::string lhs = call.substr(0, slashPos);
        const std::string rhs = call.substr(slashPos + 1);

        auto [fullEntry, fullLen] = longestPrefixMatchWithLen(call);
        auto [lhsEntry,  lhsLen] = longestPrefixMatchWithLen(lhs);
        auto [rhsEntry,  rhsLen] = longestPrefixMatchWithLen(rhs);

        const PrefixEntry* best    = nullptr;
        int                bestLen = 0;
        if (fullLen > bestLen) { best = fullEntry; bestLen = fullLen; }
        if (lhsLen  > bestLen) { best = lhsEntry;  bestLen = lhsLen;  }
        if (rhsLen  > bestLen) { best = rhsEntry;  bestLen = rhsLen;  }

        if (best) return applyOverrides(*best);
    }

    // 3. Standard longest-prefix match on the full callsign.
    auto [entry, len] = longestPrefixMatchWithLen(call);
    if (entry) return applyOverrides(*entry);

    return DxccEntity::invalid();
}

} // namespace TwoPLogger
