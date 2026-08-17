#pragma once

namespace ui {

/**
 * Reserve the off-screen frame buffer. Call once at boot, before Wi-Fi comes
 * up: the buffer needs one contiguous ~115 KB block, and on the classic ESP32
 * the TLS stack fragments the heap badly enough that a later claim fails.
 * Safe to skip — drawing falls back to painting the panel directly.
 */
void radarDisplayPrepare();

/** Draw the static sonar/radar grid (black disc, green overlay, labels). */
void radarDisplayDraw();

/** Redraw aircraft only (blits cached grid; no full-screen clear). */
void radarDisplayRefreshAircraft();

/**
 * Index into services::adsb::aircraftList() of the drawn aircraft nearest the
 * given panel-space point, or -1 when nothing sits within max_px. Only
 * aircraft inside the outer ring are candidates — rim dots are direction
 * cues, not positions, so they are not selectable.
 */
int radarDisplayHitTest(int panel_x, int panel_y, int max_px);

}  // namespace ui
