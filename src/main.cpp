/**
 * Plane Radar — WiFi setup, then radar UI on the round GC9A01 display.
 */

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"
#include "services/radar_location.h"
#include "services/wifi_setup.h"
#include "ui/flight_detail.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/status_screens.h"

namespace {

/** How close a tap must land to an aircraft symbol to select it. */
constexpr int kTouchHitRadiusPx = 18;
/** Ignore repeat reads while a finger stays down. */
constexpr unsigned long kTouchDebounceMs = 250;

bool g_radar_visible = false;
unsigned long g_wifi_down_since = 0;
unsigned long g_last_reconnect_ms = 0;
unsigned long g_last_adsb_fetch_ms = 0;
unsigned long g_last_touch_ms = 0;

void showRadarIfConnected();

/**
 * One tap opens the detail card for the nearest aircraft; the next tap
 * anywhere closes it. Taps on empty sky are ignored so a stray touch does not
 * blank the radar.
 */
void handleTouch() {
  int32_t tx = 0;
  int32_t ty = 0;
  if (!tft.getTouch(&tx, &ty)) {
    return;
  }
  if (millis() - g_last_touch_ms < kTouchDebounceMs) {
    return;
  }
  g_last_touch_ms = millis();

  if (config::kTouchDebugMarker) {
    Serial.printf("touch: %d,%d\n", static_cast<int>(tx), static_cast<int>(ty));
    tft.fillSmoothCircle(static_cast<int>(tx), static_cast<int>(ty), 4,
                         tft.color565(255, 255, 0));
  }

  if (ui::flightDetailVisible()) {
    ui::flightDetailHide();
    showRadarIfConnected();
    return;
  }

  if (!g_radar_visible) {
    return;
  }

  const int hit = ui::radarDisplayHitTest(static_cast<int>(tx),
                                          static_cast<int>(ty),
                                          kTouchHitRadiusPx);
  if (hit < 0) {
    return;
  }
  if (static_cast<size_t>(hit) >= services::adsb::aircraftCount()) {
    return;
  }
  ui::flightDetailShow(services::adsb::aircraftList()[hit]);
}

void showRadarIfConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    g_radar_visible = false;
    return;
  }
  ui::radarDisplayDraw();
  g_radar_visible = true;
}

void onRangeTap() {
  ui::radar::rangeNext();
  char range_label[12];
  ui::radar::formatCurrentRing3Label(range_label, sizeof(range_label));
  Serial.printf("Range: %s (outer ~%.0f km)\n", range_label,
                ui::radar::rangeCurrent().outer_km);

  if (g_radar_visible && WiFi.status() == WL_CONNECTED) {
    ui::radarDisplayDraw();
  }
}

void handleBootButton() {
  bootButtonPollLongPress();
  if (bootButtonConsumeTap()) {
    onRangeTap();
  }
}

void fetchAndDrawAircraft() {
  const float fetch_km = ui::radar::fetchRadiusKm();
  if (!services::adsb::fetchUpdate(services::location::lat(),
                                   services::location::lon(), fetch_km)) {
    handleBootButton();
    return;
  }
  ui::radarDisplayRefreshAircraft();
  handleBootButton();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Plane Radar");

  bootButtonInit();
  displayInit();
  // Claim the frame buffer while the heap is still unfragmented (pre-Wi-Fi).
  ui::radarDisplayPrepare();
  if (wifiShowsSetupScreenOnBoot()) {
    statusScreenPortal();
  }
  services::location::init();
  ui::radar::rangeInit();
  services::adsb::setPollFn(wifiLoop);

  if (wifiSetupConnect()) {
    showRadarIfConnected();
  }
}

void loop() {
  handleBootButton();
  handleTouch();
  wifiLoop();

  if (WiFi.status() != WL_CONNECTED) {
    if (g_radar_visible) {
      Serial.println("WiFi lost — will reconnect");
      g_radar_visible = false;
    }

    if (g_wifi_down_since == 0) {
      g_wifi_down_since = millis();
    }

    const unsigned long down_ms = millis() - g_wifi_down_since;
    if (down_ms >= config::kWifiDownGraceMs &&
        millis() - g_last_reconnect_ms >= config::kWifiReconnectIntervalMs) {
      g_last_reconnect_ms = millis();
      if (wifiReconnect()) {
        g_wifi_down_since = 0;
        showRadarIfConnected();
      }
    }
  } else {
    g_wifi_down_since = 0;
    if (ui::flightDetailVisible()) {
      // Leave the card alone; polling would repaint the radar underneath it.
    } else if (!g_radar_visible) {
      showRadarIfConnected();
    } else if (millis() - g_last_adsb_fetch_ms >= config::kAdsbFetchIntervalMs) {
      g_last_adsb_fetch_ms = millis();
      fetchAndDrawAircraft();
    }
  }

  delay(10);
}
