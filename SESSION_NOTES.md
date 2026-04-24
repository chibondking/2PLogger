# 2PLogger — Session Notes

Working log of decisions, rationales, and lessons learned across development sessions.
Maintained alongside CLAUDE.md. CLAUDE.md is the spec; this file is the history.

---

## Project Identity

- **Display name**: 2PLogger (operator's callsign: WT2P)
- **C++ namespace**: `TwoPLogger` — `namespace 2PLogger` is invalid C++ (digit prefix)
- **CMake executable**: `2plogger`
- **CMake libraries**: `2plogger_log`, `2plogger_dxcc`, `2plogger_radio`, etc.
- **CLAUDE.md still says "HamLogger (working title)"** — that name is superseded. Ignore it.

---

## Build Environment

| Item | Value |
|---|---|
| OS | Linux (WSL2, kernel 6.6.87) |
| Compiler | GCC 13.3.0, C++20 |
| Qt | 6.4.2 (Core, Widgets, Network, Sql + SQLite driver) |
| CMake | 3.28.3 |
| Build system | Ninja 1.11.1 |
| Test framework | Catch2 v3.5.3 (FetchContent) |

Build directory: `build/` (out-of-source, gitignored).

---

## Session 1 — Bootstrap & Scaffold

**What was done**
- Created `.gitignore`, `README.md`
- Verified directory structure matches CLAUDE.md layout
- Full CMake scaffold: root `CMakeLists.txt`, 11 module `CMakeLists.txt` files
- One STATIC library per module (`2plogger_app`, `2plogger_radio`, etc.)
- Stub `.h`/`.cpp` for every module — compilable but no real logic
- Catch2 test suite wired through CTest
- Clean build confirmed: `ninja && ctest` green

**Decisions**
- `CMAKE_AUTOMOC ON`, `CMAKE_AUTOUIC OFF` (no .ui files per CLAUDE.md), `CMAKE_AUTORCC ON`
- All includes rooted at `${CMAKE_SOURCE_DIR}/src` so `#include "log/LogBook.h"` works everywhere
- Catch2 fetched via FetchContent if not system-installed (v3.5.3 tag)

**Naming fix mid-session**
- User corrected: project is "2PLogger", not "HamLogger"
- `sed` rename left two lines in `src/CMakeLists.txt` still saying `hamlogger` (the executable `target_include_directories` and `target_link_libraries`). Found and fixed manually.
- Lesson: always verify `sed` renames with a targeted `grep` before declaring done.

---

## Session 2 — Database Layer

**What was done**
- `src/log/QsoRecord.h` — pure C++20 structs: `ContestData`, `QsoRecord`, `LogRecord`
- `src/log/DatabaseManager.h/.cpp` — owns all SQL, named connections
- `src/log/LogBook.h/.cpp` — domain layer above the DB
- `tests/log/` — separate executable with its own `main()` providing `QCoreApplication`
- 7 Catch2 tests, all passing

**Key design decisions**

*Pure C++20 structs for domain types*
`QsoRecord`, `ContestData`, `LogRecord` have zero Qt includes. This lets them be used in Qt-free unit tests and keeps the domain model decoupled from the Qt infrastructure. Pattern carried forward to `DxccEntity`, `RadioMode`, `RadioProfile`.

*Named QSqlDatabase connections*
Qt's `QSqlDatabase` uses a global connection registry keyed by name. Using the default name means two `DatabaseManager` instances in the same process share a connection and close each other's database. Fixed with a static atomic counter generating unique names: `"2plogger_1"`, `"2plogger_2"`, etc. The destructor calls `QSqlDatabase::removeDatabase(m_connectionName)` after `close()`.

*ISO-8601 UTC string storage*
All timestamps stored as ISO-8601 strings (e.g., `"2025-11-05T13:07:00Z"`). Conversion between `std::chrono::system_clock::time_point` and `QString` goes through `QDateTime::fromSecsSinceEpoch` / `toUTC().toString(Qt::ISODate)` inside `DatabaseManager.cpp` only. Nothing outside DatabaseManager touches raw SQL date strings.

*Timestamp precision in tests*
`QDateTime::toString(Qt::ISODate)` truncates to 1-second precision. Tests use a `floorToSec()` helper that does `std::chrono::floor<std::chrono::seconds>(tp)` before comparing round-tripped timestamps. Without this, sub-second test timestamps fail equality checks.

*LEFT JOIN contest data detection*
`fetchQsos()` uses `LEFT JOIN qso LEFT JOIN qso_contest_data`. Detecting whether a contest data row exists: check `q.isNull(25)` (the `cd.points` column). Even though `points` is `NOT NULL DEFAULT 0` in the schema, a `LEFT JOIN` miss makes it SQL NULL in the result set. Do not use `q.isNull(19)` (the first cd column, `qso_id`) — use a non-nullable column from the joined table that has a known column index.

*Transactions for multi-table writes*
`insertQso()` wraps the `qso` insert + optional `qso_contest_data` insert in a single transaction. If the contest data insert fails, the QSO row is rolled back. This keeps the database consistent even if the app crashes between the two writes.

**Bugs found and fixed**
- `#include <QWarning>` does not exist → use `#include <QtGlobal>` for `qWarning()`
- Test included `LogBook.h` but tried to instantiate `DatabaseManager` directly → needed `#include "log/DatabaseManager.h"` in the test

---

## Session 3 — DXCC Lookup Module

**What was done**
- `src/dxcc/DxccEntity.h` — pure C++20 struct, no Qt
- `src/dxcc/DxccDatabase.h/.cpp` — full AD1C cty.dat parser + longest-prefix-match lookup
- `src/dxcc/CMakeLists.txt` — removed Qt dependency (module is now pure C++20)
- `data/cty.dat` — downloaded (had to be done manually; automated fetch got HTTP 403)
- `tests/dxcc/` — 18 Catch2 tests covering all lookup cases
- 25/25 tests passing (7 database + 18 dxcc)

**cty.dat format (AD1C)**

Header line:
```
Name: CQ: ITU: Cont: Lat: Lon: UTC: Prefix:
```
Alias lines (indented, comma-separated, terminated by `;`):
```
    VK9C,VK9FC,=VK9BSA,UA9F(17)[30];
```

Override syntax within alias tokens:
- `(n)` — CQ zone override
- `[n]` — ITU zone override
- `<f>` — UTC offset override (rare; ignored per-alias in our model)
- `{XX}` — continent override (not seen in current cty.dat)
- `=` prefix — exact callsign match (not prefix match)
- `*` before entity prefix — deleted DXCC entity

**Longitude sign convention**
cty.dat stores longitude as **positive = West** (opposite of standard geographic convention). We negate on load so `DxccEntity.lon` is stored as **positive = East** (matching the comment in the header and standard GIS convention).

Verified:
- Monaco: cty.dat `lon = -7.40` → stored as `7.40` (Monaco is ~7.4°E ✓)
- Australia: cty.dat `lon = -132.33` → stored as `132.33` (Australia is ~132°E ✓)
- United States: cty.dat `lon = 91.87` → stored as `-91.87` (US is ~92°W ✓)

**Lookup algorithm**
1. Uppercase the callsign
2. Exact map lookup (`=` entries) — highest priority
3. If callsign contains `/`: try longestPrefixMatch on full call, LHS, and RHS separately; take the candidate with the longest matched prefix. This correctly resolves `W6RGG/VK9X` → Christmas Island (RHS "VK9X" matches 4-char prefix "VK9X") vs `K1XX/P` → USA (LHS "K1XX" matches "K", RHS "P" matches nothing useful).
4. Standard longestPrefixMatch on the full callsign.

**Two test failures that were actually wrong test expectations**
- `=VK9BSA` is under **Cocos (Keeling) Islands** (VK9C) in cty.dat, not Christmas Island. Fixed test.
- `UA9F(17)[30]` is aliased under **European Russia** (UA), not Asiatic Russia. UA9F stations are in the European Russia entity with zone overrides. Fixed test.
- Lesson: always verify test expectations against the actual data file before writing assertions.

---

## Session 4 — Radio Interface (rigctld backend)

**What was done**
- `src/radio/RadioMode.h` — pure C++20 enum class + `modeFromString`/`modeToString` inline functions
- `src/radio/RadioProfile.h` — pure C++20 struct with correct defaults
- `src/radio/RadioInterface.h/.cpp` — abstract QObject base, rewritten with full signal set
- `src/radio/RigctldRadio.h/.cpp` — full async implementation
- `src/radio/AetherSdrRadio.h/.cpp` — thin stub subclass, `SpotPainter*` placeholder
- `src/radio/RadioFactory.h/.cpp` — factory function
- `tests/radio/test_radio_mode.cpp` — 7 pure C++20 Catch2 tests
- 32/32 tests passing

**rigctld wire protocol**

Commands sent:
| Purpose | Wire string |
|---|---|
| Get frequency | `f\n` |
| Get mode | `m\n` |
| Set VFO | `+\set_vfo VFOA\n` |
| Set frequency | `F <hz>\n` |
| Set mode | `M <mode> 0\n` |
| PTT on/off | `T 1\n` / `T 0\n` |
| Send CW | `+\send_morse <text>\n` |
| Abort CW | `+\stop_morse\n` |

Response format:
- `f` → `<hz>\nRPRT 0\n`
- `m` → `<mode>\n<passband>\nRPRT 0\n`
- Set commands → `RPRT 0\n`
- Error → `RPRT -<n>\n` (replaces data lines)

The `+` prefix on `\send_morse` / `\stop_morse` makes rigctld return `RPRT 0` immediately without waiting for CW transmission to finish. This is essential — `\send_morse` without `+` would block the TCP connection for the entire CW duration.

**Command queue design**
All socket I/O goes through a `QQueue<Command>`. The queue enforces strict sequencing: we never write the next command until `RPRT` is received for the previous one. This is necessary because rigctld is a synchronous request-response protocol over a single TCP connection.

Application commands (`qsy`, `setMode`, etc.) push to the queue and call `drain()`. Poll tick commands also push to the queue, but only if the queue is already empty (prevents poll buildup when the rig is slow).

`qsy()` pushes two commands: `+\set_vfo VFOA` first, then `F <hz>`. They execute atomically from rigctld's perspective because the connection is sequential.

**Frequency delta filter (10 Hz)**
The poll loop emits `frequencyChanged` only when `|new - current| >= 10 Hz`. Without this filter, every 200ms poll would emit the signal even when the rig hasn't moved, causing the bandmap to re-render and the UI to flicker. The 10 Hz threshold is below any meaningful frequency step but above DAC/ADC noise in most rigctld implementations.

**Exponential backoff reconnect**
On any disconnect or socket error: start a single-shot timer for `m_reconnectDelay` ms, then double it (capped at 30,000 ms). Reset to 1,000 ms on successful `connected()`. An `m_intentionalDisconnect` flag suppresses reconnect when the operator explicitly disconnects (profile switch).

Guard against double-scheduling: `onSocketError` checks `m_reconnectTimer.isActive()` before scheduling. Both `onDisconnected` and `onSocketError` can fire in sequence for the same connection failure event.

**DIGI → PKTUSB mapping**
`modeToString(DIGI)` returns `"DIGI"` (internal/display use). `RigctldRadio::setMode(DIGI)` sends `"M PKTUSB 0\n"` (rigctld wire format). This split keeps the domain type clean while correctly instructing the rig.

**MOC requires all Qt types in the header**
`RigctldRadio.h` declares `void onSocketError(QAbstractSocket::SocketError err)` as a slot. MOC processes headers without seeing the `.cpp` includes. Omitting `#include <QAbstractSocket>` from the header (relying on it being pulled in by the `.cpp`) causes MOC-generated code to fail to compile. Rule: any Qt type appearing in a class declaration (signals, slots, member types) must be included in the header, not just the `.cpp`.

---

## Module Status

| Module | Status | Tests |
|---|---|---|
| `log` | Complete | 7 passing |
| `dxcc` | Complete | 18 passing (+ 7 log = 25) |
| `radio` | Complete (RigctldRadio + AetherSdrRadio stub) | 7 passing (+ 25 = 32) |
| `contest` | Stub only | — |
| `cluster` | Stub only | — |
| `bandmap` | Stub only | — |
| `export` | Stub only | — |
| `udp` | Stub only | — |
| `cw` | Stub only | — |
| `widgets` | Stub only | — |
| `app` | Stub only | — |

**AetherSdrRadio** is a thin shell around `RigctldRadio`. The `SpotPainter*` member is a null forward-declared pointer; it will be wired up when the UDP/spot-painting session is implemented.

---

## Deferred Items

- `data/cty.dat` has a spurious `cty.dat:Zone.Identifier` file next to it (Windows alternate data stream artifact from WSL download). Not harmful but worth cleaning up: `rm "data/cty.dat:Zone.Identifier"`.
- `AetherSdrRadio::SpotPainter` — implement in UDP session.
- `CwEngine::setSpeed()` is not on `RadioInterface` yet — will need `\set_morse_speed <wpm>` command added to `RigctldRadio` when CwEngine is implemented.
- `RadioInterface` has no `setSpeed(int wpm)` method; CwEngine will talk to the concrete type or we'll add it to the interface in the CW session.

---

## Invariants (Confirmed Working)

1. **No SQL outside `DatabaseManager`** — `LogBook` and above see only domain types.
2. **No Qt in pure domain headers** — `QsoRecord.h`, `DxccEntity.h`, `RadioMode.h`, `RadioProfile.h` have zero Qt includes.
3. **UTC everywhere** — all timestamps stored as ISO-8601 UTC strings, converted at the DB boundary only.
4. **Freq in Hz** — `uint64_t` / `quint64` throughout; no floats for frequency.
5. **DXCC at log time** — entity stored in the `qso` row; re-lookup is never done at export.
6. **Fire-and-forget UDP** — not yet implemented, but the design constraint is noted.
7. **No tight coupling** — radio layer knows nothing about the log; log layer knows nothing about radio.
