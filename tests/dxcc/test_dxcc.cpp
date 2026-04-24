#include <catch2/catch_test_macros.hpp>
#include "dxcc/DxccDatabase.h"

// All tests require a real cty.dat. The CMakeLists sets CTY_DAT_PATH so the
// binary always finds the file regardless of the working directory.
#ifndef CTY_DAT_PATH
#  error "CTY_DAT_PATH must be defined by the build system"
#endif

static TwoPLogger::DxccDatabase loadDb()
{
    TwoPLogger::DxccDatabase db;
    REQUIRE(db.loadFromFile(CTY_DAT_PATH));
    REQUIRE(db.isLoaded());
    REQUIRE(db.entityCount() > 300);  // cty.dat has ~400+ entities
    return db;
}

// ── load ──────────────────────────────────────────────────────────────────────

TEST_CASE("DxccDatabase loads cty.dat without error", "[dxcc][load]")
{
    loadDb();  // assertions inside loadDb()
}

TEST_CASE("DxccDatabase returns invalid entity for empty callsign", "[dxcc][load]")
{
    auto db = loadDb();
    REQUIRE_FALSE(db.lookup("").isValid());
}

// ── simple prefix lookup ──────────────────────────────────────────────────────

TEST_CASE("VK resolves to Australia", "[dxcc][lookup]")
{
    auto db = loadDb();
    auto e  = db.lookup("VK2IO");
    REQUIRE(e.isValid());
    REQUIRE(e.name      == "Australia");
    REQUIRE(e.prefix    == "VK");
    REQUIRE(e.continent == "OC");
    REQUIRE(e.cqZone    == 30);
}

TEST_CASE("W resolves to United States", "[dxcc][lookup]")
{
    auto db = loadDb();
    auto e  = db.lookup("W1AW");
    REQUIRE(e.isValid());
    REQUIRE(e.name      == "United States");
    REQUIRE(e.continent == "NA");
    REQUIRE(e.cqZone    == 5);
}

TEST_CASE("JA resolves to Japan", "[dxcc][lookup]")
{
    auto db = loadDb();
    auto e  = db.lookup("JA1YKX");
    REQUIRE(e.isValid());
    REQUIRE(e.name      == "Japan");
    REQUIRE(e.continent == "AS");
}

// ── longest-prefix disambiguation ─────────────────────────────────────────────

TEST_CASE("VK9XX resolves to Christmas Island (VK9X), not Australia (VK)", "[dxcc][lookup][vk9]")
{
    // VK9X is a distinct DXCC entity — longer prefix wins over VK.
    auto db = loadDb();
    auto e  = db.lookup("VK9XX");
    REQUIRE(e.isValid());
    REQUIRE(e.name   == "Christmas Island");
    REQUIRE(e.prefix == "VK9X");
}

TEST_CASE("VK9C prefix resolves to Cocos (Keeling) Islands", "[dxcc][lookup][vk9]")
{
    auto db = loadDb();
    auto e  = db.lookup("VK9CI");
    REQUIRE(e.isValid());
    REQUIRE(e.name   == "Cocos (Keeling) Islands");
    REQUIRE(e.prefix == "VK9C");
}

TEST_CASE("ZL7 resolves to Chatham Islands, not New Zealand", "[dxcc][lookup]")
{
    auto db = loadDb();
    auto e  = db.lookup("ZL7ABC");
    REQUIRE(e.isValid());
    REQUIRE(e.name   == "Chatham Islands");
    REQUIRE(e.prefix == "ZL7");
}

TEST_CASE("ZL8 resolves to Kermadec Islands", "[dxcc][lookup]")
{
    auto db = loadDb();
    auto e  = db.lookup("ZL8R");
    REQUIRE(e.isValid());
    REQUIRE(e.name   == "Kermadec Islands");
}

// ── zone overrides ────────────────────────────────────────────────────────────

TEST_CASE("UA9 callsign with zone-override prefix gets correct zones", "[dxcc][lookup][zones]")
{
    // UA9F is aliased under European Russia (UA) with (17)[30] overrides in cty.dat.
    // The override means: this sub-area uses CQ zone 17 and ITU zone 30.
    auto db = loadDb();
    auto e  = db.lookup("UA9FGJ");
    REQUIRE(e.isValid());
    REQUIRE(e.name    == "European Russia");
    REQUIRE(e.cqZone  == 17);
    REQUIRE(e.ituZone == 30);
}

// ── exact callsign match (= entries) ─────────────────────────────────────────

TEST_CASE("=VK9BSA exact match resolves to Cocos (Keeling) Islands", "[dxcc][lookup][exact]")
{
    // VK9BSA is listed as =VK9BSA under VK9C (Cocos/Keeling) in cty.dat.
    auto db = loadDb();
    auto e  = db.lookup("VK9BSA");
    REQUIRE(e.isValid());
    REQUIRE(e.name   == "Cocos (Keeling) Islands");
    REQUIRE(e.prefix == "VK9C");
}

TEST_CASE("=VK9AFP exact match resolves to Christmas Island", "[dxcc][lookup][exact]")
{
    // VK9AFP is listed as =VK9AFP under VK9X (Christmas Island) in cty.dat.
    auto db = loadDb();
    auto e  = db.lookup("VK9AFP");
    REQUIRE(e.isValid());
    REQUIRE(e.name   == "Christmas Island");
    REQUIRE(e.prefix == "VK9X");
}

// ── slash callsigns ───────────────────────────────────────────────────────────

TEST_CASE("W6RGG/VK9X resolves to Christmas Island via rhs prefix", "[dxcc][lookup][slash]")
{
    auto db = loadDb();
    auto e  = db.lookup("W6RGG/VK9X");
    REQUIRE(e.isValid());
    REQUIRE(e.name   == "Christmas Island");
}

TEST_CASE("K1XX/P resolves to United States (P suffix ignored)", "[dxcc][lookup][slash]")
{
    // /P is a portable suffix — lhs K1XX (→ K → USA) should win since
    // 'P' alone matches nothing or matches a shorter prefix than 'K'.
    auto db = loadDb();
    auto e  = db.lookup("K1XX/P");
    REQUIRE(e.isValid());
    REQUIRE(e.name   == "United States");
}

// ── case insensitivity ────────────────────────────────────────────────────────

TEST_CASE("lookup is case-insensitive", "[dxcc][lookup]")
{
    auto db = loadDb();
    REQUIRE(db.lookup("vk9xx").name == db.lookup("VK9XX").name);
    REQUIRE(db.lookup("w1aw").name  == db.lookup("W1AW").name);
}

// ── deleted entities ──────────────────────────────────────────────────────────

TEST_CASE("deleted entities are still returned but flagged isDeleted", "[dxcc][lookup][deleted]")
{
    // IT9 (Sicily) is marked *IT9 in cty.dat — deleted entity.
    auto db = loadDb();
    auto e  = db.lookup("IT9VDQ");
    REQUIRE(e.isValid());
    REQUIRE(e.isDeleted);
}

// ── coordinate sanity ─────────────────────────────────────────────────────────

TEST_CASE("Australia has sensible coordinates (positive=East convention)", "[dxcc][coords]")
{
    // Australia: roughly 25°S, 133°E
    auto db = loadDb();
    auto e  = db.lookup("VK2IO");
    REQUIRE(e.lat < 0.0);         // Southern hemisphere
    REQUIRE(e.lon > 100.0);       // East of prime meridian (positive=East stored)
}

TEST_CASE("United States has sensible coordinates", "[dxcc][coords]")
{
    // USA: roughly 38°N, 92°W
    auto db = loadDb();
    auto e  = db.lookup("W1AW");
    REQUIRE(e.lat > 30.0);
    REQUIRE(e.lon < 0.0);   // West of prime meridian
}
