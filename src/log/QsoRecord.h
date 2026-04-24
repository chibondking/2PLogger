#pragma once
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

// Pure C++20 — no Qt headers. Safe to include in Qt-free unit tests.

namespace TwoPLogger {

// Mirrors qso_contest_data table.
struct ContestData {
    int64_t                    qsoId       = 0;
    std::optional<std::string> exch1;
    std::optional<std::string> exch2;
    std::optional<std::string> exch3;
    std::optional<std::string> exch4;
    std::optional<int>         serialSent;
    std::optional<int>         serialRcvd;
    int                        points      = 0;
    bool                       isMult      = false;
    std::optional<std::string> multType;
    std::optional<std::string> multValue;
};

// Mirrors qso table. Default-constructible; all optional DB columns are std::optional.
struct QsoRecord {
    int64_t     id           = 0;
    int64_t     logId        = 0;
    std::chrono::system_clock::time_point timestampUtc;  // when the QSO happened (UTC)
    std::string callsign;
    uint64_t    freqHz       = 0;                        // always exact Hz, never float
    std::string band;
    std::string mode;
    std::string rstSent      = "599";
    std::string rstRcvd      = "599";
    std::optional<int>         txPowerW;
    std::optional<std::string> dxccEntity;
    std::optional<std::string> continent;
    std::optional<int>         cqZone;
    std::optional<int>         ituZone;
    std::optional<std::string> operatorCall;
    std::optional<std::string> notes;
    bool        isDupe       = false;
    bool        isDeleted    = false;
    std::chrono::system_clock::time_point loggedAt;      // wall clock at INSERT time

    std::optional<ContestData> contestData;
};

// Mirrors log table. Represents one operating session (contest or DX period).
struct LogRecord {
    int64_t     id           = 0;
    std::chrono::system_clock::time_point createdAt;
    std::string name;
    std::string stationCall;
    std::optional<int64_t>     contestId;
    std::optional<std::string> myGrid;
    std::optional<std::string> myDxcc;
    std::optional<int>         myCqZone;
    std::optional<std::string> operatorClass;
    std::optional<int>         powerW;
    std::optional<std::string> radioProfile;
};

} // namespace TwoPLogger
