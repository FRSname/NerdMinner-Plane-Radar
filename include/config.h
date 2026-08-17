#pragma once

#include <cstdint>

#include <driver/gpio.h>

namespace config {

// --- Wi-Fi portal ---
constexpr char kPortalApName[] = "PlaneRadar-Setup";
constexpr char kPortalIp[] = "192.168.4.1";
/** mDNS host (no ".local" suffix); browser: http://plane-radar.local */
constexpr char kPortalHostname[] = "plane-radar";
constexpr char kPortalHostUrl[] = "plane-radar.local";

/** Per-attempt STA connect wait (ms); retried kWifiConnectAttempts times. */
constexpr unsigned long kWifiConnectAttemptMs = 15000;
constexpr uint8_t kWifiConnectAttempts = 3;
constexpr unsigned long kWifiPortalTimeoutSec = 0;  // 0 = no timeout while configuring
constexpr unsigned long kWifiConnectingFrameMs = 50;
/** Wait after disconnect before reconnecting (avoids portal on brief drops). */
constexpr unsigned long kWifiDownGraceMs = 4000;
/** Minimum interval between background reconnect tries. */
constexpr unsigned long kWifiReconnectIntervalMs = 15000;

// --- Board selection ---
// Exactly one BOARD_* macro comes from the PlatformIO env (see platformio.ini).
#if !defined(BOARD_SUPERMINI) && !defined(BOARD_CYD)
#define BOARD_SUPERMINI
#endif

// Button timings are board-independent.
constexpr unsigned long kBootResetHoldMs = 3000UL;
/** Ignore BOOT taps shorter than this (debounce). */
constexpr unsigned long kBootTapMinMs = 40UL;

#if defined(BOARD_CYD)

// --- BOOT button (ESP32-2432S028, active LOW) ---
// The CYD's only user button; the other one is hard-wired to RESET.
constexpr gpio_num_t kBootPin = GPIO_NUM_0;

// --- Display: ILI9341 2.8" 320×240 on HSPI (= SPI2 on the classic ESP32) ---
// RST is tied to the ESP32 EN line on this board, so the panel has no reset pin
// of its own. MISO stays unconnected: we only ever write to the panel, and
// GPIO12 is a strapping pin (MTDI) that must read low at boot.
constexpr gpio_num_t kDisplayPinRst = GPIO_NUM_NC;
constexpr gpio_num_t kDisplayPinCs = GPIO_NUM_15;
constexpr gpio_num_t kDisplayPinDc = GPIO_NUM_2;
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_13;
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_14;
/** PWM backlight, active HIGH. The round GC9A01 board has no backlight control. */
constexpr gpio_num_t kDisplayPinBacklight = GPIO_NUM_21;

// Landscape. The radar is a 240×240 square letterboxed into the panel centre.
constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
// 1 = landscape. Bit 2 (+4) is LovyanGFX's mirror flag, so 5 is "landscape,
// mirrored" — that scrambled this panel, so we are back on plain 1 while the
// large-DMA path is under test.
constexpr uint8_t kDisplayRotation = 1;

// 27 MHz, not 40: pushing the whole frame buffer in one DMA burst is a much
// heavier load than the small direct writes this board was doing before.
constexpr uint32_t kDisplaySpiWriteHz = 27000000;
// ILI9341 on this board is straight RGB with no inversion.
constexpr bool kDisplayInvert = false;
constexpr bool kDisplayRgbOrder = false;

// --- Touch: XPT2046 on its own SPI bus (VSPI/SPI3; the panel owns HSPI) ---
constexpr gpio_num_t kTouchPinSclk = GPIO_NUM_25;
constexpr gpio_num_t kTouchPinMosi = GPIO_NUM_32;
constexpr gpio_num_t kTouchPinMiso = GPIO_NUM_39;
constexpr gpio_num_t kTouchPinCs = GPIO_NUM_33;
/**
 * Left unused (-1 = always poll over SPI). Stock CYD wiring puts IRQ on
 * GPIO 36, but depending on it means no touch at all if this clone differs.
 */
constexpr gpio_num_t kTouchPinIrq = GPIO_NUM_NC;

/**
 * Draw a dot wherever a touch lands and log the raw coordinates. Turn off
 * once taps register in the right place.
 */
constexpr bool kTouchDebugMarker = false;

/**
 * Force the corner-tap calibration to run at boot even when a saved result
 * exists. Set false once the panel is calibrated; the result lives in NVS.
 */
constexpr bool kTouchForceCalibration = false;

/**
 * Affine touch calibration measured on this panel with the corner-tap
 * routine, used when NVS holds no saved result. The X axis reads inverted
 * (first value above second), which is why plain min/max bounds put taps in
 * the wrong place.
 */
constexpr uint16_t kTouchDefaultCal[8] = {3760, 233, 3794, 3796,
                                          381,  250, 349,  3779};
/**
 * Raw ADC bounds for this panel class. If taps land offset or inverted,
 * these are the four numbers to tune (swap min/max to flip an axis).
 */
constexpr uint16_t kTouchXMin = 300;
constexpr uint16_t kTouchXMax = 3900;
constexpr uint16_t kTouchYMin = 300;
constexpr uint16_t kTouchYMax = 3900;

/** RGB status LED (active LOW) — parked HIGH at boot so it stays dark. */
constexpr gpio_num_t kLedPinR = GPIO_NUM_4;
constexpr gpio_num_t kLedPinG = GPIO_NUM_16;
constexpr gpio_num_t kLedPinB = GPIO_NUM_17;

#else  // BOARD_SUPERMINI

// --- BOOT button (ESP32-C3 Super Mini, active LOW) ---
constexpr gpio_num_t kBootPin = GPIO_NUM_9;

// --- Display: GC9A01 1.28" round 240×240 (SPI) ---
constexpr gpio_num_t kDisplayPinRst = GPIO_NUM_0;
constexpr gpio_num_t kDisplayPinCs = GPIO_NUM_1;
constexpr gpio_num_t kDisplayPinDc = GPIO_NUM_10;
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_3;  // display SDA
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_4;  // display SCL
constexpr gpio_num_t kDisplayPinBacklight = GPIO_NUM_NC;

constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 240;
constexpr uint8_t kDisplayRotation = 0;

constexpr uint32_t kDisplaySpiWriteHz = 40000000;
// GC9A01 modules often need invert + BGR for correct black/green output
constexpr bool kDisplayInvert = true;
constexpr bool kDisplayRgbOrder = true;

/** No touch panel here; defined so the shared touch path still compiles. */
constexpr bool kTouchDebugMarker = false;

#endif

// --- Radar center defaults (overridden via WiFi setup portal) ---
constexpr double kDefaultRadarLat = 49.830628;
constexpr double kDefaultRadarLon = 18.280146;

/** Aircraft altitude tag: true = metres, false = feet (the ADS-B native unit). */
constexpr bool kAltitudeInMeters = true;

/** Poll adsb.fi (API public limit: 1 req/s). */
constexpr unsigned long kAdsbFetchIntervalMs = 3000;
/** Legacy scale unused — fetch uses radar::fetchRadiusKm() to screen edge. */
constexpr float kAdsbFetchRadiusScale = 1.0f;
/** false = hide aircraft with alt_baro "ground"; true = show them too. */
constexpr bool kAdsbShowGroundAircraft = false;

// --- UI colors (RGB565) — status screens ---
constexpr uint16_t kColorBlack = 0x0000;
constexpr uint16_t kColorYellow = 0xFFE0;
constexpr uint16_t kTextOnYellow = kColorBlack;
constexpr uint16_t kTextOnBlack = 0xFFFF;

}  // namespace config
