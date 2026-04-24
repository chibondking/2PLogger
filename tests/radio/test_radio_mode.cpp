#include <catch2/catch_test_macros.hpp>
#include "radio/RadioMode.h"
#include "radio/RadioProfile.h"

using namespace TwoPLogger;

// ── modeFromString ────────────────────────────────────────────────────────────

TEST_CASE("modeFromString maps standard rigctld mode strings", "[radio][mode]")
{
    REQUIRE(modeFromString("CW")    == RadioMode::CW);
    REQUIRE(modeFromString("CWR")   == RadioMode::CWR);
    REQUIRE(modeFromString("USB")   == RadioMode::USB);
    REQUIRE(modeFromString("LSB")   == RadioMode::LSB);
    REQUIRE(modeFromString("SSB")   == RadioMode::SSB);
    REQUIRE(modeFromString("AM")    == RadioMode::AM);
    REQUIRE(modeFromString("FM")    == RadioMode::FM);
    REQUIRE(modeFromString("RTTY")  == RadioMode::RTTY);
    REQUIRE(modeFromString("RTTYR") == RadioMode::RTTYR);
    REQUIRE(modeFromString("FT8")   == RadioMode::FT8);
    REQUIRE(modeFromString("FT4")   == RadioMode::FT4);
}

TEST_CASE("modeFromString maps PKT variants to DIGI", "[radio][mode]")
{
    REQUIRE(modeFromString("PKTUSB") == RadioMode::DIGI);
    REQUIRE(modeFromString("PKTLSB") == RadioMode::DIGI);
    REQUIRE(modeFromString("PKTFM")  == RadioMode::DIGI);
    REQUIRE(modeFromString("PKTAM")  == RadioMode::DIGI);
    REQUIRE(modeFromString("DIGI")   == RadioMode::DIGI);
}

TEST_CASE("modeFromString returns Unknown for unrecognised strings", "[radio][mode]")
{
    REQUIRE(modeFromString("GARBAGE")  == RadioMode::Unknown);
    REQUIRE(modeFromString("")         == RadioMode::Unknown);
    REQUIRE(modeFromString("cw")       == RadioMode::Unknown);  // case-sensitive
    REQUIRE(modeFromString("OLIVIA")   == RadioMode::Unknown);
}

// ── modeToString ──────────────────────────────────────────────────────────────

TEST_CASE("modeToString produces expected strings", "[radio][mode]")
{
    REQUIRE(modeToString(RadioMode::CW)      == "CW");
    REQUIRE(modeToString(RadioMode::CWR)     == "CWR");
    REQUIRE(modeToString(RadioMode::USB)     == "USB");
    REQUIRE(modeToString(RadioMode::LSB)     == "LSB");
    REQUIRE(modeToString(RadioMode::SSB)     == "SSB");
    REQUIRE(modeToString(RadioMode::AM)      == "AM");
    REQUIRE(modeToString(RadioMode::FM)      == "FM");
    REQUIRE(modeToString(RadioMode::RTTY)    == "RTTY");
    REQUIRE(modeToString(RadioMode::RTTYR)   == "RTTYR");
    REQUIRE(modeToString(RadioMode::FT8)     == "FT8");
    REQUIRE(modeToString(RadioMode::FT4)     == "FT4");
    REQUIRE(modeToString(RadioMode::DIGI)    == "DIGI");
    REQUIRE(modeToString(RadioMode::Unknown) == "Unknown");
}

// ── round-trip ────────────────────────────────────────────────────────────────

TEST_CASE("modeFromString(modeToString(m)) round-trips for all non-Unknown modes",
          "[radio][mode]")
{
    // Every mode that has a unique string representation round-trips correctly.
    // DIGI is excluded because modeToString(DIGI)="DIGI" and modeFromString("DIGI")=DIGI,
    // which does round-trip; but modeFromString("PKTUSB")=DIGI cannot round-trip back
    // to "PKTUSB" since modeToString(DIGI)="DIGI". That asymmetry is by design.
    const std::initializer_list<RadioMode> nonUnknown = {
        RadioMode::CW, RadioMode::CWR, RadioMode::SSB,
        RadioMode::LSB, RadioMode::USB, RadioMode::AM,
        RadioMode::FM, RadioMode::RTTY, RadioMode::RTTYR,
        RadioMode::FT8, RadioMode::FT4, RadioMode::DIGI,
    };
    for (RadioMode m : nonUnknown) {
        INFO("mode = " << modeToString(m));
        REQUIRE(modeFromString(modeToString(m)) == m);
    }
}

// ── RadioProfile defaults ─────────────────────────────────────────────────────

TEST_CASE("RadioProfile has correct default values", "[radio][profile]")
{
    RadioProfile p;
    REQUIRE(p.rigctldHost    == "127.0.0.1");
    REQUIRE(p.rigctldPort    == 4532);
    REQUIRE(p.pollIntervalMs == 200);
    REQUIRE(p.isAetherSdr    == false);
    REQUIRE(p.spotUdpPort    == 0);
    REQUIRE(p.name.empty());
}

TEST_CASE("RadioProfile fields can be set", "[radio][profile]")
{
    RadioProfile p;
    p.name         = "Home - Flex 6600";
    p.rigctldHost  = "192.168.1.50";
    p.rigctldPort  = 4532;
    p.isAetherSdr  = true;
    p.spotUdpHost  = "127.0.0.1";
    p.spotUdpPort  = 12060;

    REQUIRE(p.name        == "Home - Flex 6600");
    REQUIRE(p.rigctldHost == "192.168.1.50");
    REQUIRE(p.isAetherSdr == true);
    REQUIRE(p.spotUdpPort == 12060);
}
