# CLAUDE.md — HamLogger (working title)
> Ground-truth reference for Claude Code. Read this before writing any code.

---

## Bootstrap Tasks (Do These First, Before Any Application Code)

When starting work on a fresh repository, complete these tasks in order before
touching CMakeLists.txt or any source files:

1. **Create `.gitignore`** — must cover at minimum:
   - CMake build directories (`build/`, `cmake-build-*/`)
   - Qt build artifacts (`*.moc`, `moc_*`, `ui_*`, `qrc_*`)
   - Compiled binaries and AppImage outputs
   - IDE directories (`.vscode/`, `.idea/`, `.clangd/`)
   - macOS cruft (`.DS_Store`)
   - `*.db` files (operator log files — never commit real logs)
   - `*.user` (Qt Creator per-user project files)

2. **Create `README.md`** — must include:
   - Project name and one-paragraph description (what it is, who it's for)
   - Current status (early development / pre-alpha)
   - Feature list drawn from v1 scope in this document
   - Build prerequisites (Qt6, CMake 3.21+, Ninja, rigctld at runtime)
   - Build instructions (clone → cmake → ninja)
   - Runtime prerequisites (AetherSDR or rigctld daemon must be running for radio control)
   - UDP ecosystem overview (brief: what packets it sends/receives and why)
   - License (GPL-3.0 to match the ham radio open source ecosystem convention)
   - A note that this is WT2P's personal project, not affiliated with FlexRadio,
     CWops, or any other organization

3. **Verify the directory structure** matches the layout in this document before
   creating any source files.

---

## Project Overview

A native Linux contest and DX logging application written in Qt6/C++20. Primary target is
the operator's home station running a FlexRadio 6600 fronted by AetherSDR, with secondary
support for field operation with an IC-7300. The application is a **peer** in a UDP-based
shack ecosystem — it communicates with AetherSDR, GHEControl (antenna switch client),
PSTRotator, and future tools via UDP broadcast. There is **no tight coupling** to any other
application. Each app is independently deployable and can be absent without crashing this one.

---

## Hard Constraints

These are non-negotiable. Do not deviate without explicit instruction.

- **C++20 and Qt6 only.** No Qt5 compatibility shims. No Boost. No third-party frameworks
  beyond what is listed in Dependencies.
- **Qt modules: Core, Widgets, Network, Sql only.** No QtQuick, no QtWebEngine, no
  QtBluetooth, no QtSerialPort (CW keying goes through rigctld, not direct serial).
- **No UI files (.ui).** All UI is constructed in code. No Qt Designer artifacts.
- **No hardcoded callsigns, contest exchanges, DXCC entities, or band plans.** All such
  data is loaded from files or derived from the database at runtime.
- **No tight coupling to GHEControl, AetherSDR, or any external app.** All inter-app
  communication is fire-and-forget UDP. The logger must start and operate normally if no
  other app is running.
- **SQLite only.** No MySQL, no PostgreSQL, no external database server. The log is a
  single `.db` file the operator can copy, back up, and open with any SQLite tool.
- **CMake 3.21+ with Ninja.** No qmake.
- **Build targets: Linux AppImage, Windows installer (Inno Setup).** Linux is primary.

---

## Repository Layout

```
hamlogger/
├── CMakeLists.txt
├── CLAUDE.md                  ← this file
├── cmake/
│   └── ...                    ← find modules, toolchain files
├── src/
│   ├── main.cpp
│   ├── app/                   ← application shell, main window
│   ├── radio/                 ← RadioInterface + concrete backends
│   ├── cluster/               ← DX cluster client + spot model
│   ├── bandmap/               ← BandmapModel + BandmapWidget
│   ├── log/                   ← QsoRecord, LogBook, dupe checking
│   ├── contest/               ← ContestRules interface + CWT impl
│   ├── dxcc/                  ← cty.dat parser + DXCC lookup
│   ├── export/                ← AdifExporter, CabrilloExporter
│   ├── udp/                   ← UdpBroadcaster + UdpListener
│   ├── cw/                    ← CwEngine (sends via rigctld)
│   └── widgets/               ← EntryWidget, LogView, ScoreWidget, etc.
├── data/
│   ├── cty.dat                ← Country Files by AD1C
│   └── contests/              ← per-contest definition files (INI-style)
├── packaging/
│   ├── appimage/
│   └── inno/
└── tests/
    └── ...                    ← catch2 unit tests, no Qt required for logic tests
```

---

## Dependencies

| Dependency | Purpose | Notes |
|---|---|---|
| Qt6 Core | Signals, strings, timers, JSON | Required |
| Qt6 Widgets | All UI | Required |
| Qt6 Network | QTcpSocket (cluster, rigctld), QUdpSocket | Required |
| Qt6 Sql + SQLite driver | Log database | Required |
| Hamlib rigctld | Radio CAT (external daemon, not linked) | Runtime only — logger speaks the rigctld TCP protocol, does not link libhamlib |
| cty.dat | DXCC entity database | Bundled in data/, updated periodically |
| Catch2 | Unit tests | Build-time only |

**Hamlib is not linked.** The logger speaks the rigctld TCP wire protocol directly via
`QTcpSocket`. This means `rigctld` must be running as a daemon (for IC-7300), or
AetherSDR's built-in rigctld server must be active (for Flex). The logger does not
manage or launch rigctld.

---

## Radio Architecture

### The Problem
Two radios, two contexts:
- **Home**: FlexRadio 6600 ← AetherSDR (Linux, Qt6/C++20) → exposes rigctld TCP
- **Field**: IC-7300 → rigctld daemon (Hamlib) → exposes rigctld TCP

Both expose the **same rigctld TCP protocol on port 4532** (configurable). The logger
speaks only rigctld. It does not know or care what is underneath.

### RadioInterface (abstract)

```cpp
// src/radio/RadioInterface.h
class RadioInterface : public QObject {
    Q_OBJECT
public:
    virtual void qsy(quint64 hz) = 0;
    virtual void setMode(RadioMode mode) = 0;
    virtual void setPtt(bool tx) = 0;
    virtual void sendCw(const QString& text) = 0;
    virtual void abortCw() = 0;

signals:
    void frequencyChanged(quint64 hz);
    void modeChanged(RadioMode mode);
    void txStateChanged(bool transmitting);
    void connectionStateChanged(bool connected);
};
```

### RigctldRadio (concrete, covers both backends)

```cpp
// src/radio/RigctldRadio.h
class RigctldRadio : public RadioInterface {
    // QTcpSocket to rigctld
    // QTimer polling at configurable interval (default 200ms)
    // Implements full RadioInterface
    // sendCw() uses rigctld \send_morse command
    // abortCw() uses \stop_morse
};
```

### AetherSdrRadio (extends RigctldRadio, home station only)

```cpp
// src/radio/AetherSdrRadio.h
class AetherSdrRadio : public RigctldRadio {
    // Adds spot painting to AetherSDR panadapter
    // Sends N1MM-format UDP spot packets that AetherSDR understands
    // Everything else inherited from RigctldRadio
};
```

### RadioProfile (config)

```cpp
struct RadioProfile {
    QString     name;           // "Home - Flex 6600", "Field - IC-7300"
    QString     rigctldHost;    // "127.0.0.1"
    quint16     rigctldPort;    // 4532
    int         pollIntervalMs; // 200
    bool        isAetherSdr;    // true = use AetherSdrRadio subclass
    QString     spotUdpHost;    // AetherSDR spot target, if isAetherSdr
    quint16     spotUdpPort;
};
```

The operator selects a `RadioProfile` at startup (combo box in toolbar or settings).
The application instantiates the appropriate `RadioInterface` subclass based on
`isAetherSdr`. Switching profiles requires app restart or explicit reconnect — no
hot-swap in v1.

---

## UDP Broadcast Architecture

The logger participates in a **UDP ecosystem** of shack applications. All communication
is **fire-and-forget**. The logger does not know if anyone is listening. Absence of any
listener is not an error.

### Outbound (logger → ecosystem)

All outbound packets are **JSON over UDP**, broadcast to a configurable address
(default `127.0.0.1`, operator can set to LAN broadcast address).

#### QSO Logged packet
Sent immediately when a QSO is committed to the database.
```json
{
  "type": "QSO_LOGGED",
  "version": 1,
  "timestamp": "2025-11-05T13:07:00Z",
  "callsign": "VK9XX",
  "frequency_hz": 14025500,
  "band": "20m",
  "mode": "CW",
  "rst_sent": "599",
  "rst_rcvd": "599",
  "dxcc_entity": "VK9/C",
  "cq_zone": 28,
  "operator": "WT2P",
  "contest": "CWT",
  "exch1": "BILL",
  "exch2": "IL"
}
```

#### Radio State packet
Sent on every frequency/mode change (rate-limited to max 10/sec).
```json
{
  "type": "RADIO_STATE",
  "version": 1,
  "frequency_hz": 14025500,
  "band": "20m",
  "mode": "CW",
  "tx": false,
  "profile": "Home - Flex 6600"
}
```

#### Spot packet (re-broadcast inbound cluster spots)
```json
{
  "type": "DX_SPOT",
  "version": 1,
  "dx_call": "VK9XX",
  "spotter": "W1AW",
  "frequency_hz": 14025500,
  "band": "20m",
  "mode": "CW",
  "comment": "599",
  "timestamp": "2025-11-05T13:05:00Z",
  "dxcc_entity": "VK9/C",
  "cq_zone": 28,
  "is_new_dxcc": true,
  "is_new_band": false
}
```

### Inbound (ecosystem → logger)

The logger listens on a configurable UDP port (default `12060`) for:

#### Tune command (e.g., from a future bandmap tool or rotor controller)
```json
{
  "type": "TUNE_TO",
  "frequency_hz": 14025500,
  "mode": "CW"
}
```

#### External spot (e.g., from WSJT-X or another tool)
```json
{
  "type": "EXTERNAL_SPOT",
  "dx_call": "VK9XX",
  "frequency_hz": 14025500,
  "mode": "FT8",
  "comment": "WSJT-X decode"
}
```

### UdpBroadcaster / UdpListener classes

```cpp
// src/udp/UdpBroadcaster.h
class UdpBroadcaster : public QObject {
    // QUdpSocket, configurable target host + port
    // void broadcast(const QJsonObject& packet);
    // All sends are best-effort, no error on failure
};

// src/udp/UdpListener.h
class UdpListener : public QObject {
    // QUdpSocket bound to listen port
    // Parses incoming JSON, routes by "type" field
signals:
    void tuneToReceived(quint64 hz, RadioMode mode);
    void externalSpotReceived(DxSpot spot);
};
```

**GHEControl integration note**: When GHEControl receives a `RADIO_STATE` packet with
a band change, it can automatically select the appropriate antenna. GHEControl sends
no packets to the logger — it only listens. This is the intended one-way relationship.
The logger does not know GHEControl exists.

**PSTRotator integration note**: PSTRotator can receive UDP bearing commands. When the
logger adds rotor support in a future version, it will emit a `ROTOR_BEARING` packet.
PSTRotator listens. Same pattern.

---

## Database Schema

Single SQLite file. One file per operating session (contest or DX log period).
The operator names the file at session creation.

```sql
-- Every QSO ever logged. Contest-agnostic.
CREATE TABLE qso (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    log_id          INTEGER NOT NULL REFERENCES log(id),
    timestamp_utc   TEXT NOT NULL,          -- ISO8601, always UTC
    callsign        TEXT NOT NULL COLLATE NOCASE,
    freq_hz         INTEGER NOT NULL,       -- exact Hz
    band            TEXT NOT NULL,          -- '20m', '40m', etc. derived at log time
    mode            TEXT NOT NULL,          -- 'CW', 'SSB', 'FT8', 'RTTY'
    rst_sent        TEXT NOT NULL DEFAULT '599',
    rst_rcvd        TEXT NOT NULL DEFAULT '599',
    tx_power_w      INTEGER,
    dxcc_entity     TEXT,                   -- from cty.dat at log time, persisted
    continent       TEXT,
    cq_zone         INTEGER,
    itu_zone        INTEGER,
    operator_call   TEXT,                   -- blank = station call
    notes           TEXT,
    is_dupe         INTEGER NOT NULL DEFAULT 0,
    is_deleted      INTEGER NOT NULL DEFAULT 0,
    logged_at       TEXT NOT NULL           -- wall clock when row was inserted
);

-- Contest exchange data. NULL for DX logging sessions.
CREATE TABLE qso_contest_data (
    qso_id          INTEGER PRIMARY KEY REFERENCES qso(id) ON DELETE CASCADE,
    exch1           TEXT,   -- CWT: operator name
    exch2           TEXT,   -- CWT: state/province/country or member number
    exch3           TEXT,   -- future use
    exch4           TEXT,   -- future use
    serial_sent     INTEGER,
    serial_rcvd     INTEGER,
    points          INTEGER NOT NULL DEFAULT 0,
    is_mult         INTEGER NOT NULL DEFAULT 0,
    mult_type       TEXT,   -- 'NAME', 'SPC', 'NR', 'DXCC', 'ZONE', etc.
    mult_value      TEXT    -- the actual value that was new
);

-- Multiplier tracking. Source of truth for live score display.
-- INSERT OR IGNORE: if it inserted, it was a new mult.
CREATE TABLE contest_mult (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    log_id          INTEGER NOT NULL REFERENCES log(id),
    mult_type       TEXT NOT NULL,
    mult_value      TEXT NOT NULL COLLATE NOCASE,
    first_qso_id    INTEGER REFERENCES qso(id),
    UNIQUE(log_id, mult_type, mult_value)
);

-- A session: either a contest entry or a DX logging period.
CREATE TABLE log (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    created_at      TEXT NOT NULL,
    name            TEXT NOT NULL,          -- 'CWT 2025-11-05 1300z' or 'DX Nov 2025'
    station_call    TEXT NOT NULL,          -- WT2P
    contest_id      INTEGER REFERENCES contest(id),  -- NULL = DX logging
    my_grid         TEXT,                   -- Maidenhead
    my_dxcc         TEXT,
    my_cq_zone      INTEGER,
    operator_class  TEXT,                   -- 'SO', 'SO-LP', 'SO-QRP', etc.
    power_w         INTEGER,
    radio_profile   TEXT                    -- name of RadioProfile used
);

-- Contest definitions. One row per known contest.
CREATE TABLE contest (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    name            TEXT NOT NULL UNIQUE,   -- 'CWT'
    cabrillo_name   TEXT,                   -- 'CWops-CWT'
    def_file        TEXT                    -- path to data/contests/cwt.ini
);

-- Past exchange data for callsign pre-fill.
-- Populated from qso + qso_contest_data on QSO log. Keyed by callsign.
CREATE TABLE callsign_history (
    callsign        TEXT NOT NULL COLLATE NOCASE,
    contest_id      INTEGER NOT NULL REFERENCES contest(id),
    last_name       TEXT,
    last_spc        TEXT,
    last_nr         TEXT,
    last_seen       TEXT NOT NULL,          -- ISO8601
    PRIMARY KEY (callsign, contest_id)
);

-- Indexes
CREATE INDEX idx_qso_log_band     ON qso(log_id, band);
CREATE INDEX idx_qso_log_call     ON qso(log_id, callsign);
CREATE INDEX idx_qso_timestamp    ON qso(timestamp_utc);
CREATE INDEX idx_mult_log         ON contest_mult(log_id, mult_type);
```

---

## Contest Engine

### ContestRules Interface

```cpp
// src/contest/ContestRules.h
class ContestRules : public QObject {
    Q_OBJECT
public:
    virtual QString contestName() const = 0;
    virtual QString cabrilloName() const = 0;

    // Is this QSO a dupe given the current log?
    virtual bool isDupe(const QsoRecord& candidate,
                        const LogBook& log) const = 0;

    // Points this QSO contributes (0 for dupes)
    virtual int qsoPoints(const QsoRecord&) const = 0;

    // What exchange fields does this contest use?
    // Returns list of labels, e.g. {"Name", "State/Nr"}
    virtual QStringList exchangeFieldLabels() const = 0;

    // Validate a filled exchange. Return empty string if OK, else error message.
    virtual QString validateExchange(const QsoRecord&) const = 0;

    // Populate mult tracking from a just-logged QSO.
    // Inserts into contest_mult if new. Returns true if any new mult found.
    virtual bool updateMults(const QsoRecord&, LogBook& log) const = 0;

    // Format one QSO as a Cabrillo QSO: line
    virtual QString formatCabrilloQso(const QsoRecord&) const = 0;

    // Cabrillo header fields specific to this contest
    virtual QMap<QString, QString> cabrilloHeader(const Log& log) const = 0;
};

// Null implementation for DX logging — no contest rules apply
class NullContestRules : public ContestRules {
    bool isDupe(...) const override { return false; }
    int  qsoPoints(...) const override { return 1; }
    // exchangeFieldLabels returns {} — entry widget shows no exchange fields
    // updateMults does nothing
    // formatCabrilloQso — not applicable
};
```

### CWT Implementation

```cpp
// src/contest/CwtRules.h
class CwtRules : public ContestRules {
    // Exchange: Name (exch1) + SPC-or-Nr (exch2)
    // Dupe: once per band (mode axis irrelevant, always CW)
    // Points: 1 per QSO
    // Mults: unique Names (type='NAME') + unique SPCs/Nrs (type='SPC' or 'NR')
    //        both are GLOBAL across all bands (not per-band)
    // Score: total QSOs × total mults
    //
    // exch2 classification:
    //   numeric string → mult_type = 'NR' (CWOps member number)
    //   alpha string   → mult_type = 'SPC' (state/province/country)
    //
    // Cabrillo name: CWops-CWT
    // QSO line format:
    //   QSO: 14025 CW 2025-11-05 1301 WT2P         599 BILL          IL
    //   QSO: 7021  CW 2025-11-05 1307 WT2P         599 JOHN          1234
};
```

### Contest Definition Files

Stored in `data/contests/`. INI-style text. Used for future contests added without
recompiling. CWT is the first implementation — hardcoded in C++ as `CwtRules`,
definition file provides metadata only.

```ini
; data/contests/cwt.ini
[contest]
name = CWT
cabrillo_name = CWops-CWT
exchange_fields = Name, State/Province/Country or Member Nr
dupe_per_band = true
dupe_per_mode = false
scoring = QSO_POINTS * TOTAL_MULTS
mult_types = NAME, SPC, NR
mult_scope = GLOBAL
```

---

## DX Cluster / Bandmap

### ClusterClient

```cpp
// src/cluster/ClusterClient.h
class ClusterClient : public QObject {
    Q_OBJECT
    // QTcpSocket, line-buffered telnet-style connection
    // Supports: DXSpider, AR Cluster, CC Cluster (common DX de format)
    // Login: send callsign on connect, then optional set commands
    // Reconnect: exponential backoff (1s, 2s, 4s, ... max 60s)
    // Spot line parser: "DX de SPOTTER: FREQ  DXCALL  COMMENT  TIME"
signals:
    void spotReceived(DxSpot spot);
    void connectionStateChanged(bool connected);
    void rawLineReceived(QString line);  // for cluster chat window
};
```

### DxSpot

```cpp
// src/cluster/DxSpot.h
struct DxSpot {
    QString     dxCall;
    QString     spotter;
    quint64     freqHz;
    QString     band;           // derived from freqHz
    RadioMode   mode;           // inferred from freq/comment, may be Unknown
    QString     comment;
    QDateTime   timestamp;      // UTC
    QString     dxccEntity;     // from cty.dat lookup
    QString     continent;
    int         cqZone;
    bool        isNewDxcc;      // computed against current log
    bool        isNewBand;      // computed against current log
    SpotAge     age;            // Fresh (<10min) / Recent (<30min) / Old

    // Spot dedup key: dxCall + band
    QString dedupKey() const { return dxCall + band; }
};
```

### BandmapModel

```cpp
// src/bandmap/BandmapModel.h
class BandmapModel : public QAbstractTableModel {
    Q_OBJECT
    // Internal storage: QMap<QString, DxSpot> keyed by dedupKey()
    // Per-band view: QSortFilterProxyModel subclass
    // Aging: QTimer fires every 60 seconds, updates SpotAge, emits dataChanged
    // Dupe status: recomputed on every QSO_LOGGED signal
    // addSpot(): dedup by key, reset age if same call+band seen again
    // Columns: Freq | Callsign | DXCC | Zone | Age | Comment
public slots:
    void addSpot(const DxSpot& spot);
    void onQsoLogged(const QsoRecord& qso);  // triggers dupe recompute
    void onRadioFrequencyChanged(quint64 hz); // highlights nearest spot
signals:
    void spotSelected(DxSpot spot);  // user clicked — caller does QSY + fill entry
};
```

### BandmapWidget

Renders `BandmapModel`. Frequency on Y axis, descending. Color coding:
- **New DXCC** (`#ff6b35` orange): not worked in any log
- **New band** (`#4ecdc4` teal): worked DXCC but not this band
- **Worked** (`#888888` gray): already in log this session
- **Fresh** vs **Recent** vs **Old**: brightness/opacity variation on above

Click a spot → `BandmapModel::spotSelected` → `RadioInterface::qsy()` + fill
callsign entry field. Widget does not talk to the radio directly.

---

## CW Engine

CW is sent via rigctld's `\send_morse <text>` command. The `CwEngine` class:

```cpp
// src/cw/CwEngine.h
class CwEngine : public QObject {
    Q_OBJECT
    // Holds reference to RadioInterface (not owned)
    // sendMessage(QString text): expands macros, calls radioInterface->sendCw()
    // abort(): calls radioInterface->abortCw()
    // setSpeed(int wpm): sends rigctld \set_morse_speed command
    //
    // Macro expansion:
    //   {CALL}    → current callsign in entry field
    //   {NAME}    → exch1 field
    //   {NR}      → serial number (contest) or QSO number (DX)
    //   {MYCALL}  → station callsign from log config
    //   {RST}     → rst_sent field
    //   {QSL}     → "TU" (configurable)
    //
    // Function key messages F1-F8: stored in contest/profile config
    // F1 = CQ message, F2 = exchange, F3 = TU, etc.
};
```

CW speed control: keyboard shortcuts `+` / `-` adjust WPM, displayed in status bar.

**No hardware serial keying in v1.** CW goes through rigctld exclusively. This means
rigctld (or AetherSDR's rigctld server) must support `\send_morse`. AetherSDR already
does. For IC-7300 in the field, Hamlib's rigctld supports it for most Icom rigs.

---

## DXCC / cty.dat

```cpp
// src/dxcc/DxccDatabase.h
class DxccDatabase {
    // Loaded once at startup from data/cty.dat
    // AD1C format (standard cty.dat)
    // lookup(callsign) → DxccEntity
    // Uses prefix matching with exception list
    // Returns "Unknown" entity struct if no match (never returns null)
};

struct DxccEntity {
    QString name;       // "Australia"
    QString prefix;     // "VK"
    QString continent;  // "OC"
    int     cqZone;
    int     ituZone;
    double  lat;
    double  lon;
    float   utcOffset;
    bool    isDeleted;  // deleted DXCC entities
};
```

**Important**: cty.dat is loaded at startup and does not change during a session.
DXCC entity is stored on the `qso` row at log time. Export uses the stored value,
not a re-lookup. This ensures historical accuracy if cty.dat is updated later.

---

## Export

### ADIF

```cpp
// src/export/AdifExporter.h
class AdifExporter {
    // Exports QList<QsoRecord> to ADIF 3.x format
    // Always includes: CALL, QSO_DATE, TIME_ON, BAND, MODE, FREQ,
    //                  RST_SENT, RST_RCVD, DXCC, CQZ, ITUZ, CONT,
    //                  MY_GRIDSQUARE, OPERATOR, TX_PWR
    // Contest fields when present: STX, SRX, NAME, STATE (from exch1/exch2)
    // exportToFile(QList<QsoRecord>, QString path) → bool
    // exportToString(QList<QsoRecord>) → QString
    // singleRecord(QsoRecord) → QString  (for real-time upload hooks, future)
};
```

### Cabrillo

```cpp
// src/export/CabrilloExporter.h
class CabrilloExporter {
    // Requires a ContestRules* — calls formatCabrilloQso() per QSO
    // Generates header from Log metadata + ContestRules::cabrilloHeader()
    // exportToFile(Log, QList<QsoRecord>, ContestRules*, QString path) → bool
    //
    // CWT header fields:
    //   START-OF-LOG: 3.0
    //   CONTEST: CWops-CWT
    //   CALLSIGN: WT2P
    //   CATEGORY-OPERATOR: SINGLE-OP
    //   CATEGORY-POWER: HIGH (derived from log.power_w)
    //   CLAIMED-SCORE: <computed>
    //   OPERATORS: WT2P
    //   NAME: <from settings>
    //   ADDRESS: <from settings>
    //   SOAPBOX:
    //   END-OF-LOG:
};
```

---

## Entry Widget Behavior

The entry widget is the heart of the UI. Keyboard flow must be fast.

### DX Logging mode
```
[Callsign field] → Enter → logs QSO with current freq/mode/time
```

### Contest mode (CWT)
```
[Callsign] Tab → [Name] Tab → [SPC/Nr] Enter → logs QSO
```

**Pre-fill behavior** (CWT): When callsign field loses focus or Tab is pressed,
query `callsign_history` for this callsign + contest. If found, pre-fill Name
and SPC/Nr fields. Operator can override. This is the killer feature for CWT
regulars who use the same name and number every week.

**Dupe detection**: As callsign is typed (debounced 300ms), query `qso` table
for same callsign, same `log_id`, same band. If found:
- Entry field background → `#3a1a1a` (dark red)
- Status bar shows "DUPE — worked [timestamp] on [band]"
- QSO can still be logged (operator override), but is flagged `is_dupe = 1`

**Partial callsign match**: As callsign is typed, show matching callsigns from
`callsign_history` in a small popup list (like autocomplete). Selecting one fills
the callsign field and triggers pre-fill.

**ESM (Enter Sends Message)** — v1 optional:
- First Enter with callsign filled but exchange empty → sends F2 (exchange) CW
- Second Enter with exchange filled → logs QSO and sends F3 (TU)

---

## Visual Design

Match AetherSDR's dark aesthetic. Same color vocabulary where it makes sense —
the operator is looking at both apps simultaneously.

| Element | Value |
|---|---|
| Window background | `#1e1e1e` |
| Panel/widget background | `#252525` |
| Input field background | `#2d2d2d` |
| Primary text | `#e0e0e0` |
| Dim text / labels | `#888888` |
| Accent / frequency | `#4fc3f7` (light blue) |
| New DXCC spot | `#ff6b35` (orange) |
| New band spot | `#4ecdc4` (teal) |
| Worked/dupe spot | `#555555` (gray) |
| Dupe entry field | `#3a1a1a` (dark red) |
| QSO logged flash | `#1a3a1a` (dark green, brief) |
| CW sending indicator | `#ffeb3b` (yellow) |

Font: monospace for frequency display and log view. System default for labels.

---

## Main Window Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ Toolbar: [Profile ▾] [Band ▾] [Mode ▾] [Speed: 28 WPM ±] [Freq]│
├──────────────────────────┬──────────────────────────────────────┤
│                          │                                      │
│   BANDMAP                │   LOG VIEW                           │
│   (BandmapWidget)        │   (recent QSOs, reverse chron)       │
│                          │                                      │
│                          │                                      │
├──────────────────────────┴──────────────────────────────────────┤
│  ENTRY:  [Callsign_______] [Name____] [SPC/Nr__]  [RST S/R]    │
│  F1:CQ   F2:Exch   F3:TU   F4:   F5:   F6:   F7:   F8:        │
├─────────────────────────────────────────────────────────────────┤
│ Score: 47 QSOs × 63 mults = 2961  │ Rate: 72/hr │ 00:23 left   │
│ Cluster: connected  Radio: 14.025  │ Status messages            │
└─────────────────────────────────────────────────────────────────┘
```

Score bar and rate display are visible only when a contest log is active.
DX logging mode shows simplified status (DXCC count, confirmed count).

---

## Configuration

All configuration is stored in `~/.config/hamlogger/hamlogger.ini` (QSettings).
No registry on Linux. The log `.db` file is separate from config.

Config sections:
- `[station]` — callsign, name, grid, address (for Cabrillo headers)
- `[radio_profiles]` — array of RadioProfile structs
- `[active_profile]` — name of selected RadioProfile
- `[cluster]` — host, port, login callsign, set commands
- `[udp]` — broadcast address, broadcast port, listen port
- `[cw]` — default speed, F-key messages per contest
- `[ui]` — window geometry, splitter positions

---

## Key Invariants

1. **The log is the source of truth.** UI state is always derived from the database,
   not held in memory independently. On crash/restart, the log is intact.

2. **UTC everywhere.** All timestamps stored and displayed in UTC. No local time
   in the database. UI may show local time as a secondary display only.

3. **Freq in Hz.** All internal frequency representation is `quint64` in Hz.
   Band strings (`"20m"`) are derived, never stored as primary data except as
   a denormalized convenience column on `qso`.

4. **DXCC at log time.** `dxcc_entity`, `continent`, `cq_zone`, `itu_zone` are
   written to the `qso` row when the QSO is logged, not derived at export time.

5. **UDP is best-effort.** Never block the UI waiting for UDP acknowledgment.
   Never retry UDP sends. Never treat a missing listener as an error.

6. **No tight coupling.** The logger does not import, link, or subprocess-launch
   GHEControl, AetherSDR, PSTRotator, or any other shack application.

7. **Contest rules are pluggable.** No contest-specific logic outside of
   `src/contest/`. The entry widget, log view, and score widget all take a
   `ContestRules*` and behave appropriately for whatever it returns.

8. **DX logging is the null contest.** `NullContestRules` makes DX logging a
   special case of the contest engine, not a separate code path.

---

## v1 Scope (What to Build)

### In scope
- RadioInterface + RigctldRadio + AetherSdrRadio (rigctld backend)
- RadioProfile selection (home Flex / field IC-7300)
- DX cluster client (DXSpider compatible)
- BandmapModel + BandmapWidget (single band view, all bands toggle)
- QSO entry widget with dupe detection and CWT pre-fill
- Log view (scrollable table of recent QSOs)
- ContestRules interface + NullContestRules + CwtRules
- SQLite schema (full schema above, even if not all tables used in v1)
- CwEngine (rigctld send_morse, macro expansion, F1-F8)
- DXCC lookup from cty.dat
- ADIF export
- Cabrillo export (CWT format)
- UDP broadcaster (QSO_LOGGED, RADIO_STATE, DX_SPOT)
- UDP listener (TUNE_TO, EXTERNAL_SPOT)
- Score display widget (CWT: QSOs × mults, running rate)
- Settings dialog (station info, radio profiles, cluster, UDP ports, CW messages)
- Linux AppImage packaging

### Explicitly out of scope for v1
- Voice keying / DVK
- SO2R / SO2V
- Rotor control / PSTRotator integration (UDP packet defined but not sent)
- WSJT-X / FT8 integration
- LoTW / eQSL upload
- DX cluster chat window (raw lines received but not displayed)
- Network multi-op
- Any contest other than CWT (and DX logging via NullContestRules)
- Windows installer (AppImage first, Inno Setup later)
- AetherSDR panadapter spot painting (UDP packet format defined, sending deferred)

---

## What "Done" Looks Like for v1

The operator can:
1. Launch the app, select "Home - Flex 6600" profile
2. AetherSDR's rigctld server is running — radio connects, frequency displays
3. Cluster connects, spots appear in bandmap, color-coded by DXCC status
4. Click a bandmap spot → radio QSYs (via AetherSDR rigctld), callsign fills
5. Create a new CWT log, set station info
6. During CWT: type call, Tab pre-fills Name/SPC from history, Enter logs
7. Dupe detection highlights in real time as callsign is typed
8. F1 sends CQ, F2 sends exchange, F3 sends TU — all via rigctld to AetherSDR
9. Score widget shows live QSO count × mult count × rate
10. After CWT: File → Export Cabrillo, submit to CWops
11. File → Export ADIF, import to LoTW/ClubLog manually
12. Switch to "Field - IC-7300" profile (rigctld daemon running), same workflow
