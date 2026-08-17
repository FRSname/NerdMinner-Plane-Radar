#pragma once

namespace services::route {

/** Origin/destination for a callsign, as far as adsbdb knows it. */
struct Route {
  char origin[8];
  char origin_city[28];
  char destination[8];
  char destination_city[28];
};

/**
 * Blocking lookup of a flight's route by callsign against api.adsbdb.com.
 * Returns false when the callsign is empty, unknown to the database, or the
 * request fails — `out` is cleared either way, so a false return is simply
 * "no route to show", not an error the caller must handle.
 */
bool lookup(const char* callsign, Route* out);

}  // namespace services::route
