#pragma once
#include <string>

// Pure C++20 — no Qt headers. Safe to include in Qt-free unit tests.

namespace TwoPLogger {

enum class RadioMode {
    CW, CWR, SSB, LSB, USB, AM, FM, RTTY, RTTYR,
    FT8, FT4, DIGI, Unknown
};

// Map rigctld mode strings to RadioMode.
// PKTUSB/PKTLSB/PKTFM/PKTAM → DIGI. Anything unrecognised → Unknown.
inline RadioMode modeFromString(const std::string& s)
{
    if (s == "CW")     return RadioMode::CW;
    if (s == "CWR")    return RadioMode::CWR;
    if (s == "USB")    return RadioMode::USB;
    if (s == "LSB")    return RadioMode::LSB;
    if (s == "SSB")    return RadioMode::SSB;
    if (s == "AM")     return RadioMode::AM;
    if (s == "FM")     return RadioMode::FM;
    if (s == "RTTY")   return RadioMode::RTTY;
    if (s == "RTTYR")  return RadioMode::RTTYR;
    if (s == "FT8")    return RadioMode::FT8;
    if (s == "FT4")    return RadioMode::FT4;
    if (s == "PKTUSB") return RadioMode::DIGI;
    if (s == "PKTLSB") return RadioMode::DIGI;
    if (s == "PKTFM")  return RadioMode::DIGI;
    if (s == "PKTAM")  return RadioMode::DIGI;
    if (s == "DIGI")   return RadioMode::DIGI;
    return RadioMode::Unknown;
}

// Convert RadioMode to its canonical string.
// For DIGI, returns "DIGI" (not "PKTUSB") — use this for display/storage.
// RigctldRadio::setMode() maps DIGI → "PKTUSB" for the wire format separately.
inline std::string modeToString(RadioMode mode)
{
    switch (mode) {
        case RadioMode::CW:      return "CW";
        case RadioMode::CWR:     return "CWR";
        case RadioMode::SSB:     return "SSB";
        case RadioMode::LSB:     return "LSB";
        case RadioMode::USB:     return "USB";
        case RadioMode::AM:      return "AM";
        case RadioMode::FM:      return "FM";
        case RadioMode::RTTY:    return "RTTY";
        case RadioMode::RTTYR:   return "RTTYR";
        case RadioMode::FT8:     return "FT8";
        case RadioMode::FT4:     return "FT4";
        case RadioMode::DIGI:    return "DIGI";
        case RadioMode::Unknown: return "Unknown";
    }
    return "Unknown";
}

} // namespace TwoPLogger
