#include "services/route_client.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <cstring>

namespace services::route {

namespace {

constexpr char kApiBase[] = "https://api.adsbdb.com/v0/callsign/";
constexpr unsigned long kConnectTimeoutMs = 6000;
constexpr unsigned long kRequestTimeoutMs = 8000;

void copyField(const JsonObject& obj, const char* key, char* out,
               size_t out_len) {
  const char* s =
      obj[key].is<const char*>() ? obj[key].as<const char*>() : nullptr;
  if (s == nullptr) {
    return;
  }
  strncpy(out, s, out_len - 1);
  out[out_len - 1] = '\0';
}

/** Prefer the IATA code travellers recognise; fall back to ICAO. */
void copyAirport(const JsonObject& airport, char* code, size_t code_len,
                 char* city, size_t city_len) {
  copyField(airport, "iata_code", code, code_len);
  if (code[0] == '\0') {
    copyField(airport, "icao_code", code, code_len);
  }
  copyField(airport, "municipality", city, city_len);
  if (city[0] == '\0') {
    copyField(airport, "name", city, city_len);
  }
}

}  // namespace

bool lookup(const char* callsign, Route* out) {
  memset(out, 0, sizeof(*out));

  if (callsign == nullptr || callsign[0] == '\0' ||
      WiFi.status() != WL_CONNECTED) {
    return false;
  }

  String url = kApiBase;
  url += callsign;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    return false;
  }
  http.setConnectTimeout(kConnectTimeoutMs);
  http.setTimeout(kRequestTimeoutMs);

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    // 404 is the ordinary "no route on file" answer, not worth logging.
    if (code != HTTP_CODE_NOT_FOUND) {
      Serial.printf("route: HTTP %d for %s\n", code, callsign);
    }
    http.end();
    return false;
  }

  // Keep only the two airport objects; the full reply also carries airline
  // and aircraft blocks this panel never shows.
  JsonDocument filter;
  JsonObject route_filter = filter["response"]["flightroute"].to<JsonObject>();
  for (const char* side : {"origin", "destination"}) {
    JsonObject side_filter = route_filter[side].to<JsonObject>();
    side_filter["iata_code"] = true;
    side_filter["icao_code"] = true;
    side_filter["municipality"] = true;
    side_filter["name"] = true;
  }

  JsonDocument doc;
  const DeserializationError err = deserializeJson(
      doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("route: parse failed (%s)\n", err.c_str());
    return false;
  }

  JsonObject flightroute = doc["response"]["flightroute"];
  if (flightroute.isNull()) {
    return false;
  }

  JsonObject origin = flightroute["origin"];
  JsonObject destination = flightroute["destination"];
  if (!origin.isNull()) {
    copyAirport(origin, out->origin, sizeof(out->origin), out->origin_city,
                sizeof(out->origin_city));
  }
  if (!destination.isNull()) {
    copyAirport(destination, out->destination, sizeof(out->destination),
                out->destination_city, sizeof(out->destination_city));
  }

  return out->origin[0] != '\0' || out->destination[0] != '\0';
}

}  // namespace services::route
