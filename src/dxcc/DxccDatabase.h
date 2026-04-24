#pragma once
#include "dxcc/DxccEntity.h"
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// Pure C++20 — no Qt headers. Safe to include in Qt-free unit tests.

namespace TwoPLogger {

class DxccDatabase {
public:
    bool loadFromFile(const std::string& path);
    DxccEntity lookup(const std::string& callsign) const;
    bool isLoaded()    const { return m_loaded; }
    int  entityCount() const { return static_cast<int>(m_entities.size()); }

private:
    struct PrefixEntry {
        std::size_t entityIdx      = 0;
        int         cqZoneOverride = 0;   // 0 = use entity default
        int         ituZoneOverride = 0;  // 0 = use entity default
    };

    std::vector<DxccEntity>                      m_entities;
    std::map<std::string, PrefixEntry>           m_prefixMap;  // sorted; enables longest-prefix search
    std::unordered_map<std::string, PrefixEntry> m_exactMap;   // = entries
    bool m_loaded = false;

    DxccEntity                         applyOverrides(const PrefixEntry& e) const;
    std::pair<const PrefixEntry*, int> longestPrefixMatchWithLen(const std::string& call) const;
};

} // namespace TwoPLogger
