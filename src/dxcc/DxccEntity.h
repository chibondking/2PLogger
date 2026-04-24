#pragma once
#include <string>

// Pure C++20 — no Qt headers. Safe to include in Qt-free unit tests.

namespace TwoPLogger {

struct DxccEntity {
    std::string name;       // e.g. "Australia"
    std::string prefix;     // e.g. "VK"  (the entity's canonical DXCC prefix)
    std::string continent;  // two-letter code: AF AS EU NA OC SA AN
    int         cqZone    = 0;
    int         ituZone   = 0;
    double      lat       = 0.0;   // decimal degrees, positive = North
    double      lon       = 0.0;   // decimal degrees, positive = East
    float       utcOffset = 0.0f;  // hours from UTC
    bool        isDeleted = false; // deleted DXCC entities

    bool isValid() const { return !name.empty(); }

    static DxccEntity invalid() { return {}; }
};

} // namespace TwoPLogger
