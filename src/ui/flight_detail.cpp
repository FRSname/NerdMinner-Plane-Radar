#include "ui/flight_detail.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"
#include "services/radar_location.h"
#include "services/route_client.h"
#include "ui/radar_theme.h"

namespace ui {

namespace {

constexpr int kMarginX = 10;
constexpr int kMarginY = 8;
constexpr int kPadX = 10;
constexpr int kPadY = 8;
constexpr int kCorner = 8;
constexpr int kRowGap = 3;

constexpr float kKmPerDeg = 111.0f;
constexpr float kKnotToKmh = 1.852f;

constexpr auto& kFontTitle = fonts::FreeSansBold12pt7b;
constexpr auto& kFontBody = fonts::FreeSans9pt7b;

bool s_visible = false;

uint16_t s_bg = 0;
uint16_t s_border = 0;
uint16_t s_label = 0;
uint16_t s_value = 0;

void initPanelPalette() {
  s_bg = tft.color565(8, 16, 40);
  s_border = tft.color565(40, 120, 60);
  s_label = tft.color565(120, 150, 190);
  s_value = tft.color565(255, 255, 255);
}

/** Flat-earth distance is fine over the tens of km this radar covers. */
float distanceKm(float lat, float lon) {
  const float dx =
      static_cast<float>(lon - services::location::lon()) * kKmPerDeg;
  const float dy =
      static_cast<float>(lat - services::location::lat()) * kKmPerDeg;
  return sqrtf(dx * dx + dy * dy);
}

/** Compass bearing from the radar centre to the aircraft, 0-359°. */
int bearingDeg(float lat, float lon) {
  const float dx =
      static_cast<float>(lon - services::location::lon()) * kKmPerDeg;
  const float dy =
      static_cast<float>(lat - services::location::lat()) * kKmPerDeg;
  float deg = atan2f(dx, dy) / radar::kDegToRad;
  if (deg < 0.0f) {
    deg += 360.0f;
  }
  return static_cast<int>(lroundf(deg)) % 360;
}

int rowHeight() { return tft.fontHeight(); }

/** One "Label            value" row; returns the y for the next row. */
int drawRow(int x, int y, int right, const char* label, const char* value) {
  displayFontSetBitmap(tft, &kFontBody);
  const int h = rowHeight();

  tft.setTextDatum(textdatum_t::top_left);
  tft.setTextColor(s_label, s_bg);
  tft.drawString(label, x, y);

  tft.setTextDatum(textdatum_t::top_right);
  tft.setTextColor(s_value, s_bg);
  tft.drawString(value, right, y);

  return y + h + kRowGap;
}

}  // namespace

bool flightDetailVisible() { return s_visible; }

void flightDetailHide() { s_visible = false; }

void flightDetailShow(const services::adsb::Aircraft& plane) {
  initPanelPalette();
  s_visible = true;

  const int x0 = kMarginX;
  const int y0 = kMarginY;
  const int w = config::kDisplayWidth - 2 * kMarginX;
  const int h = config::kDisplayHeight - 2 * kMarginY;

  tft.fillRoundRect(x0, y0, w, h, kCorner, s_bg);
  tft.drawRoundRect(x0, y0, w, h, kCorner, s_border);

  const int left = x0 + kPadX;
  const int right = x0 + w - kPadX;
  int y = y0 + kPadY;

  // Title: callsign, falling back to the ICAO address which is always present.
  displayFontSetBitmap(tft, &kFontTitle);
  tft.setTextDatum(textdatum_t::top_left);
  tft.setTextColor(s_value, s_bg);
  const char* title = plane.callsign[0] != '\0' ? plane.callsign : plane.hex;
  tft.drawString(title, left, y);
  y += tft.fontHeight() + kRowGap + 2;

  char buf[48];

  if (plane.type[0] != '\0' || plane.reg[0] != '\0') {
    snprintf(buf, sizeof(buf), "%s%s%s", plane.type,
             (plane.type[0] != '\0' && plane.reg[0] != '\0') ? "  " : "",
             plane.reg);
    y = drawRow(left, y, right, "Aircraft", buf);
  }

  if (plane.alt[0] != '\0') {
    y = drawRow(left, y, right, "Altitude", plane.alt);
  }

  snprintf(buf, sizeof(buf), "%d km/h",
           static_cast<int>(lroundf(plane.gs_knots * kKnotToKmh)));
  y = drawRow(left, y, right, "Speed", buf);

  if (fabsf(plane.vert_rate_fpm) >= 1.0f) {
    // Feet/min from the feed, shown in m/s to match the metric altitude.
    const float ms = plane.vert_rate_fpm * 0.3048f / 60.0f;
    snprintf(buf, sizeof(buf), "%+.1f m/s", static_cast<double>(ms));
    y = drawRow(left, y, right, "Climb", buf);
  }

  snprintf(buf, sizeof(buf), "%d km  %d\xC2\xB0",
           static_cast<int>(lroundf(distanceKm(plane.lat, plane.lon))),
           bearingDeg(plane.lat, plane.lon));
  y = drawRow(left, y, right, "Distance", buf);

  snprintf(buf, sizeof(buf), "%d\xC2\xB0",
           static_cast<int>(lroundf(plane.track_deg)));
  y = drawRow(left, y, right, "Track", buf);

  if (plane.squawk[0] != '\0') {
    y = drawRow(left, y, right, "Squawk", plane.squawk);
  }

  // The route needs a second API call, so say it is coming before the request
  // blocks — otherwise the panel just sits there looking finished.
  displayFontSetBitmap(tft, &kFontBody);
  tft.setTextDatum(textdatum_t::top_left);
  tft.setTextColor(s_label, s_bg);
  tft.drawString("Route...", left, y);

  services::route::Route route;
  const bool have_route = services::route::lookup(plane.callsign, &route);

  tft.fillRect(left, y, right - left, rowHeight(), s_bg);
  if (have_route) {
    snprintf(buf, sizeof(buf), "%s > %s",
             route.origin[0] != '\0' ? route.origin : "?",
             route.destination[0] != '\0' ? route.destination : "?");
    y = drawRow(left, y, right, "Route", buf);

    if (route.origin_city[0] != '\0' || route.destination_city[0] != '\0') {
      snprintf(buf, sizeof(buf), "%s > %s", route.origin_city,
               route.destination_city);
      displayFontSetBitmap(tft, &kFontBody);
      tft.setTextDatum(textdatum_t::top_left);
      tft.setTextColor(s_label, s_bg);
      tft.drawString(buf, left, y);
    }
  } else {
    y = drawRow(left, y, right, "Route", "unknown");
  }

  tft.setTextDatum(textdatum_t::top_left);
}

}  // namespace ui
