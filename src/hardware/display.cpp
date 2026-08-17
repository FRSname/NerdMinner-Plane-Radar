#include "hardware/display.h"

#include <Arduino.h>
#include <Preferences.h>

#include <cstring>

#include "config.h"
#include "hardware/display_font.h"

LGFX tft;

namespace {

#if defined(BOARD_CYD)

constexpr char kPrefsNamespace[] = "radar";
constexpr char kKeyTouchCal[] = "touchcal";
constexpr size_t kCalValues = 8;

void logCalibration(const uint16_t* cal) {
  Serial.print("touch cal:");
  for (size_t i = 0; i < kCalValues; ++i) {
    Serial.printf(" %u", cal[i]);
  }
  Serial.println();
}

/**
 * Restore the saved touch calibration, or run the corner-tap routine once and
 * keep the result. Raw XPT2046 readings vary panel to panel, so measuring
 * beats guessing at min/max bounds.
 */
void touchInit() {
  uint16_t cal[kCalValues] = {0};

  if (config::kTouchForceCalibration) {
    tft.fillScreen(config::kColorBlack);
    tft.setTextDatum(textdatum_t::top_center);
    tft.setTextColor(config::kTextOnBlack, config::kColorBlack);
    tft.drawString("Tap each corner marker", config::kDisplayWidth / 2, 8);

    tft.calibrateTouch(cal, config::kTextOnBlack, config::kColorBlack, 20);

    Preferences prefs;
    prefs.begin(kPrefsNamespace, false);
    prefs.putBytes(kKeyTouchCal, cal, sizeof(cal));
    prefs.end();

    tft.setTouchCalibrate(cal);
    logCalibration(cal);
    return;
  }

  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  const bool saved =
      prefs.getBytesLength(kKeyTouchCal) == sizeof(cal) &&
      prefs.getBytes(kKeyTouchCal, cal, sizeof(cal)) == sizeof(cal);
  prefs.end();

  if (!saved) {
    // Fall back to the values measured on this panel rather than blocking
    // boot on a calibration screen.
    memcpy(cal, config::kTouchDefaultCal, sizeof(cal));
  }

  tft.setTouchCalibrate(cal);
  logCalibration(cal);
}

#endif  // BOARD_CYD

/** Board housekeeping that must happen before the panel comes up. */
void boardInit() {
#if defined(BOARD_CYD)
  // The RGB LED is active LOW, so it glows on a floating pin. Park it dark.
  for (const gpio_num_t pin :
       {config::kLedPinR, config::kLedPinG, config::kLedPinB}) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }
#endif
}

}  // namespace

void displayInit() {
  boardInit();
  tft.init();
  tft.setRotation(config::kDisplayRotation);
  tft.setBrightness(255);
  tft.setTextWrap(false);
  displayFontInit();
#if defined(BOARD_CYD)
  touchInit();
#endif
}
