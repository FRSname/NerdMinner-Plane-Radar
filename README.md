# Plane Radar

![NerdMinner Plane Radar](https://raw.githubusercontent.com/FRSname/NerdMinner-Plane-Radar/main/NerdMinner-Plane-Radar.gif)

**3D printed case (STL + assembly):** [MakerWorld](https://makerworld.com/en/models/2872376-esp32-plane-radar-live-ads-b-on-a-round-display#profileId-3207083) · **Firmware:** [Releases](https://github.com/MatixYo/ESP32-Plane-Radar/releases)

Firmware showing a circular **ADS-B radar** around your configured location, with **WiFiManager** for first-time setup.

> **This is a fork** of [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar) adding support for
> **ESP32-2432S028-class boards** ("Cheap Yellow Display" and NerdMiner TPM408-2.8 clones), plus tap-to-inspect,
> configurable radar orientation, and metric altitudes. The original ESP32-C3 target is unchanged.

## Supported boards

| Env | Board | Display | Notes |
|-----|-------|---------|-------|
| `supermini` | ESP32-C3 Super Mini | 1.28″ round GC9A01 240×240 | Upstream target |
| `cyd` | ESP32-2432S028 (ESP32-WROOM-32) | 2.8″ ILI9341 320×240 | |
| `cyd-st7789` | Same board, later revision | 2.8″ ST7789 320×240 | Incl. NerdMiner TPM408-2.8 |

The two 2.8″ revisions look identical from outside. If the image is mirrored or sheared on `cyd`, flash
`cyd-st7789` — the controllers share `MADCTL`/`CASET`/`RASET`, so the wrong driver half-works rather than
failing outright.

## Quick start (NerdMiner / 2.8″ board)

`cyd-st7789` is the default env, so no `-e` flag is needed:

```bash
pio run -t upload
pio device monitor
```

Then:

1. Connect to the **`PlaneRadar-Setup`** Wi‑Fi AP and open `http://plane-radar.local`
2. Enter your Wi‑Fi credentials and your latitude/longitude, and save
3. Tap the four corner markers if the touch calibration screen appears

Prebuilt images are attached to [Releases](https://github.com/FRSname/NerdMinner-Plane-Radar/releases) —
take `plane-radar-cyd-st7789-*.bin` and flash it at **0x1000** with
[esptool-js](https://espressif.github.io/esptool-js/). `FLASHING.md` in each release lists the offsets.

## What it does

1. **Wi‑Fi setup** (if needed) — captive portal on AP **`PlaneRadar-Setup`**
2. **Radar** — live aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid

After Wi‑Fi is saved, the device reconnects automatically; the radar runs in the main loop with periodic ADS-B updates (~5 s).

## Controls

BOOT button — **GPIO 9** on the Super Mini, **GPIO 0** on the 2.8″ boards; active LOW either way.

| Action | Effect |
|--------|--------|
| **Short tap** | Cycle range preset (5 → 10 → 15 → 25 km); saved to flash |
| **Hold 3 s** | Clear Wi‑Fi, location, and units; reboot into setup portal |

During setup you can also hold BOOT at power-on to force a credential reset (same as the long press).

On the 2.8″ boards the touchscreen adds **tap an aircraft** → flight detail card, **tap again** → dismiss.
The long press does *not* clear the touch calibration.

## Wi‑Fi setup portal

**First-time setup** (no saved Wi‑Fi):

1. Connect to **`PlaneRadar-Setup`**
2. Open **`http://plane-radar.local`** (preferred) or **`http://192.168.4.1`** — both are shown on the yellow setup screen; captive portal may open automatically
3. Set home Wi‑Fi, then save

**Reconfigure anytime** (after the device is on your network):

1. Open **`http://plane-radar.local`** or **`http://<device-ip>`** (e.g. from your router or serial log at boot)
2. Change Wi‑Fi, location, units, or runway overlay; save

The same portal runs on the setup AP and on the device’s LAN IP while connected to Wi‑Fi. mDNS hostname is `plane-radar` → **plane-radar.local** (`kPortalHostname` in `config.h`). Some clients resolve `.local` slowly; use the IP if needed.

**Custom fields** (stored in NVS):

| Field | Purpose |
|-------|---------|
| **Latitude / Longitude** | Radar center and ADS-B query position (defaults in `config.h` until set) |
| **Display distances in miles** | Ring scale label in **mi** instead of **km** (e.g. `6mi` vs `10km`) |
| **Show airport runways** | Major-airport runway overlay on the radar (off to hide) |

After a reset, the device reboots and shows the setup screen immediately (no “Connecting” loop on stale credentials).

## Radar display

### Grid

- Dark blue background, subdued green rings and crosshairs
- White **N / S / E / W** at the bezel; range label on the **east** spoke (ring 3 = ¾ of outer radius)
- White center dot

Layout and colors: `include/ui/radar_theme.h`.

### Range presets

| Ring 3 label | Outer radius (aircraft scale) |
|------------|-------------------------------|
| 5 km / 3 mi | ~6.7 km |
| 10 km / 6 mi | ~13.3 km (default) |
| 15 km / 9 mi | ~20 km |
| 25 km / 16 mi | ~33.3 km |

Preset and miles/km choice persist across reboot (`planeradar` NVS namespace).

### Runways

- Major airports from OurAirports (`large_airport`); all open runway strips in range (helipads excluded)
- Teal runway lines with one ICAO label per airport (e.g. `KJFK`); toggle in the Wi‑Fi setup portal
- Update the embedded list: `python3 scripts/build_large_airports.py`

### Aircraft

- **Inside the outer ring** — red heading triangle, magenta speed vector (clipped at the ring), callsign / type / altitude tags
- **Outside the ring** (still within ADS-B fetch) — small **red dot on the screen rim** at the correct bearing (direction cue; not distance-accurate past the ring)
- **Tags** — placed toward the **center**: west (left) → tag on the **right** of the symbol; east (right) → tag on the **left**

As range decreases (or aircraft approach), targets move inward; beyond-ring dots become full symbols when they cross the outer ring.

### Tap to inspect (2.8″ boards)

Tapping an aircraft symbol opens a detail card over the radar; any tap dismisses it. Radar polling pauses
while it is open so nothing repaints underneath.

| Field | Source |
|-------|--------|
| Callsign (or ICAO hex) | adsb.fi `flight` / `hex` |
| Type + registration | `t` / `r` |
| Altitude, speed, climb | `alt_baro`, `gs`, `baro_rate` — shown in m, km/h, m/s |
| Distance + bearing | Computed from your configured location |
| Track, squawk | `track`, `squawk` |
| Origin → destination | [api.adsbdb.com](https://www.adsbdb.com/) (second request, on tap) |

Routes are looked up per tap and block briefly; GA and cargo flights often have none and show `unknown`.

Touch is an **XPT2046** on its own SPI bus. Calibration is measured once by tapping four corner markers and
kept in NVS, with the measured values compiled in as a fallback (`kTouchDefaultCal`). To recalibrate, set
`kTouchForceCalibration = true` in `config.h`. `kTouchDebugMarker` draws a dot at each touch and logs raw
coordinates, which is the fastest way to tell "touch is dead" from "touch is miscalibrated".

### Radar orientation

`kScreenUpBearingDeg` in `include/ui/radar_theme.h` sets which compass bearing points at the top of the
screen — `0` for conventional north-up, `90` for east-up (this fork's default). Aircraft projection, rim
dots, heading vectors, runway overlay and the N/E/S/W labels all derive from that single value.

### ADS-B

- Source: `https://opendata.adsb.fi/api/v3/`
- Fetch radius: `ui::radar::fetchRadiusKm()` — scales with the active preset to roughly the screen edge (so rim dots have data)
- Poll interval: `kAdsbFetchIntervalMs` (5 s) in `config.h`
- Ground aircraft hidden by default (`kAdsbShowGroundAircraft`)

## Configuration

Edit **`include/config.h`** for hardware and behavior:

| Area | Keys / notes |
|------|----------------|
| Portal | `kPortalApName`, `kPortalIp`, `kPortalHostname` / `kPortalHostUrl` (mDNS; needs `-DWM_MDNS` in `platformio.ini`) |
| Wi‑Fi timing | connect attempts, reconnect grace, portal timeout (`0` = no timeout) |
| BOOT | `kBootPin`, `kBootResetHoldMs`, `kBootTapMinMs` |
| Display SPI | pins, `kDisplayInvert`, `kDisplayRgbOrder`, `kDisplaySpiWriteHz` |
| Default location | `kDefaultRadarLat`, `kDefaultRadarLon` (until portal overrides) |
| ADS-B | `kAdsbFetchIntervalMs`, `kAdsbShowGroundAircraft` |
| Altitude units | `kAltitudeInMeters` (`true` = metres, `false` = feet as the feed reports them) |
| Touch (2.8″) | `kTouchPin*`, `kTouchDefaultCal`, `kTouchForceCalibration`, `kTouchDebugMarker` |
| Rotation | `kDisplayRotation` — bit 2 (`+4`) is LovyanGFX's mirror flag, so `5` is "landscape, mirrored" |

Range presets: `include/ui/radar_range.h` (`kRangePresets`).

## Project layout

```
include/
  config.h
  hardware/
    lgfx_config.hpp
    display.h
    display_font.h
  data/
    large_airports.h
  ui/
    radar_theme.h            — layout, colors, screen-up bearing
    radar_range.h
    radar_display.h
    runway_overlay.h
    status_screens.h
    flight_detail.h          — tap-to-inspect card
  services/
    wifi_setup.h
    radar_location.h
    adsb_client.h
    route_client.h           — origin/destination lookup
data/
  ui_font.vlw              — embedded smooth UI font (Noto Sans Bold)
scripts/
  build_large_airports.py
src/
  main.cpp
  data/
    large_airports_data.cpp
  hardware/
  ui/
  services/
```

## Wiring

### GC9A01 ↔ ESP32-C3 Super Mini

| Display | ESP32-C3 |
|---------|----------|
| VCC | 3V3 |
| GND | GND |
| RST | GPIO **0** |
| CS | GPIO **1** |
| DC | GPIO **10** |
| SDA (MOSI) | GPIO **3** |
| SCL (SCLK) | GPIO **4** |
| BOOT (user) | GPIO **9** |

### ESP32-2432S028 (already wired on the board)

| Function | GPIO | | Function | GPIO |
|----------|------|-|----------|------|
| TFT SCLK | **14** | | Touch CLK | **25** |
| TFT MOSI | **13** | | Touch CS | **33** |
| TFT CS | **15** | | Touch DIN | **32** |
| TFT DC | **2** | | Touch DO | **39** |
| TFT RST | tied to EN | | Touch IRQ | unused |
| Backlight | **21** (PWM) | | BOOT | **0** |

The panel is on HSPI and touch on VSPI, so they never contend. Touch IRQ is deliberately not used
(`pin_int = -1`, always poll) — relying on it means no touch at all if a clone wires it differently.

## Build

```bash
pio run -e cyd-st7789 -t upload
pio device monitor
```

Use `-e supermini` or `-e cyd` for the other boards. Serial is **115200** baud.

Two build details worth knowing if you adapt this:

- `build_unflags = -std=gnu++11` is required — the Arduino framework appends that *after* `build_flags`,
  silently overriding `-std=gnu++17`.
- LovyanGFX is pinned to **1.2.27**, not caret-ranged. Newer releases declare a global `namespace fonts`
  that collides with the aliases this project used.

### Web-flashable release image

Single `.bin` for [esptool-js](https://espressif.github.io/esptool-js/) and similar tools (ESP32-C3, 4 MB). **The flash offset depends on the chip:** **0x0** on the ESP32-C3, **0x1000** on the classic ESP32, whose ROM loader reserves the first 4 KB. `scripts/merge_firmware.py` derives this from `build.mcu`, so the merged image is already correct for whichever env you built.

```bash
chmod +x scripts/merge-firmware.sh   # once
./scripts/merge-firmware.sh
```

Writes `release/plane-radar-merged.bin`. Skip rebuild if firmware is already built:

```bash
./scripts/merge-firmware.sh --no-build
```

Or via PlatformIO only (output: `.pio/build/supermini/firmware-merged.bin`):

```bash
pio run -e supermini
pio run -t merge -e supermini
```

Put the board in download mode (hold **BOOT**, tap **RESET**), then flash with Chrome/Edge over USB.

### CI and releases (GitHub Actions)

| Workflow | When | Output |
|----------|------|--------|
| [Build](.github/workflows/build.yml) | Push / PR to `main` | One artifact per env: `plane-radar-cyd-st7789`, `plane-radar-cyd`, `plane-radar-supermini` (merged + split `.bin`, ~90 days) |
| [Release](.github/workflows/release.yml) | Git tag `v*` (e.g. `v1.0.0`) | `plane-radar-<env>-v1.0.0.bin` + `.sha256` for all three envs, plus `FLASHING.md` with the per-board offsets |

To ship a version users can download:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The release workflow builds firmware in CI and attaches the merged image to the release. Download from **Releases** on GitHub, then flash at the offset for your chip (see `FLASHING.md` in the release, or the table above). The C3 image is `plane-radar-supermini-*.bin` (ESP32-C3, 4 MB).

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) — pinned to 1.2.27, see [Build](#build)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)

Data sources: [adsb.fi](https://opendata.adsb.fi/) for aircraft, [adsbdb](https://www.adsbdb.com/) for flight
routes, [OurAirports](https://ourairports.com/) for the embedded runway dataset.
