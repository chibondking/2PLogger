# 2PLogger

2PLogger is a native Linux contest and DX logging application written in Qt6/C++20,
built for the amateur radio operator running a FlexRadio 6600 with AetherSDR or an
IC-7300 in the field. It is a peer in a UDP-based shack ecosystem — exchanging
real-time packets with AetherSDR, GHEControl (antenna switch), and PSTRotator — while
remaining independently deployable. No other application needs to be running for the
logger to start and operate.

**Status: early development / pre-alpha. Not usable yet.**

---

## Features (v1 scope)

- **Radio control** via rigctld TCP protocol — works with AetherSDR's built-in
  rigctld server (FlexRadio 6600) or Hamlib's rigctld daemon (IC-7300 and others)
- **Switchable radio profiles** — home station (Flex/AetherSDR) and field station
  (IC-7300) with per-profile host, port, and poll interval
- **DX cluster client** — connects to DXSpider-compatible cluster nodes, parses
  spots, enriches with DXCC data
- **Band map** — frequency-sorted spot display with color coding: new DXCC (orange),
  new band (teal), already worked (gray); click to QSY
- **DXCC lookup** — cty.dat (AD1C format) loaded at startup; entity, zone, and
  continent stored on every QSO at log time
- **QSO entry widget** — fast keyboard-driven entry with real-time dupe detection;
  callsign history pre-fills exchange fields in repeat contests
- **CWT contest support** — dupe-per-band, Name + SPC/Nr exchange, live
  QSOs × mults score, Cabrillo export to CWops-CWT format
- **DX logging** — null contest mode: no exchange, no mults, ADIF export
- **CW keying** — via rigctld `\send_morse`; F1–F8 macro messages with callsign,
  name, serial, and exchange substitution; `+`/`-` speed adjust
- **UDP ecosystem integration** — broadcasts `QSO_LOGGED`, `RADIO_STATE`, and
  `DX_SPOT` packets; listens for `TUNE_TO` and `EXTERNAL_SPOT`
- **ADIF export** — ADIF 3.x format for LoTW, ClubLog, and other services
- **SQLite log** — single `.db` file per session; no server required
- **Linux AppImage** packaging

---

## Build Prerequisites

| Requirement | Version |
|---|---|
| Qt6 (Core, Widgets, Network, Sql + SQLite driver) | 6.2 or later |
| CMake | 3.21 or later |
| Ninja | any recent |
| C++ compiler | GCC 11+ or Clang 13+ (C++20 required) |

Runtime only (not linked — logger speaks the TCP protocol directly):

- **rigctld** (Hamlib) for IC-7300 and other rigs, **or**
- **AetherSDR** with its built-in rigctld server for FlexRadio

---

## Build Instructions

```bash
git clone https://github.com/WT2P/2PLogger.git
cd 2PLogger
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

The resulting binary is `build/src/2plogger`. An AppImage build target will be added
once the application reaches alpha.

---

## Runtime Prerequisites

Radio control requires either:

- **Home station**: AetherSDR running with its rigctld server enabled (default port 4532)
- **Field station**: `rigctld` daemon running against the IC-7300 or other rig

The logger will start and operate normally without a radio connected — frequency
display will show "not connected" and CW will be disabled until the radio backend
comes online.

No other shack application (GHEControl, PSTRotator, etc.) is required.

---

## UDP Ecosystem

2PLogger participates in a fire-and-forget UDP ecosystem. All packets are JSON.
The logger does not know or care whether any listener is present.

**Outbound (logger → ecosystem):**

| Packet | Trigger | Consumers |
|---|---|---|
| `QSO_LOGGED` | Every committed QSO | GHEControl, future tools |
| `RADIO_STATE` | Every freq/mode change (max 10/sec) | GHEControl (band → antenna), AetherSDR |
| `DX_SPOT` | Inbound cluster spot re-broadcast | Future bandmap tools |

**Inbound (ecosystem → logger):**

| Packet | Action |
|---|---|
| `TUNE_TO` | QSY radio to specified frequency and mode |
| `EXTERNAL_SPOT` | Add spot to band map (e.g., from WSJT-X) |

Default broadcast address: `127.0.0.1`. Configurable to LAN broadcast for
multi-machine shacks. Listen port default: `12060`.

---

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).

GPL-3.0 is used to align with the ham radio open source ecosystem convention
(Hamlib, WSJT-X, fldigi, and similar projects).

---

## Disclaimer

This is WT2P's personal shack software project. It is not affiliated with,
endorsed by, or supported by FlexRadio Systems, CWops, Hamlib, or any other
organization. Use at your own risk.
