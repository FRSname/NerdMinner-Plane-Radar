#pragma once

#include <cstddef>

namespace services::adsb {

struct Aircraft {
  float lat;
  float lon;
  float nose_deg;
  float track_deg;
  float gs_knots;
  /** Climb (+) or descent (-) in feet per minute; 0 when not reported. */
  float vert_rate_fpm;
  char callsign[9];
  char type[5];
  char alt[12];
  /** ICAO 24-bit address, the only always-present identifier. */
  char hex[8];
  /** Tail number, when the feed carries one. */
  char reg[12];
  char squawk[6];
};

constexpr size_t kMaxAircraft = 64;

size_t aircraftCount();
const Aircraft* aircraftList();

/** Hook invoked during long HTTP I/O (e.g. wifiLoop). Optional. */
using PollFn = void (*)();
void setPollFn(PollFn fn);

/** Fetch aircraft within fetch_radius_km of center_lat/lon from adsb.fi. */
bool fetchUpdate(double center_lat, double center_lon, float fetch_radius_km);

}  // namespace services::adsb
