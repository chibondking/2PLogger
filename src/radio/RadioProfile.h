#pragma once
#include <cstdint>
#include <string>

// Pure C++20 — no Qt headers.

namespace TwoPLogger {

struct RadioProfile {
    std::string name;
    std::string rigctldHost    = "127.0.0.1";
    uint16_t    rigctldPort    = 4532;
    int         pollIntervalMs = 200;
    bool        isAetherSdr    = false;
    std::string spotUdpHost;
    uint16_t    spotUdpPort    = 0;
};

} // namespace TwoPLogger
