#pragma once

#include "services/adsb_client.h"

namespace ui {

/**
 * Draw the detail card for one aircraft over the radar. Takes a copy of the
 * aircraft: the ADS-B list is refreshed on a timer and would otherwise move
 * under the panel while it is open. Blocks briefly on the route lookup.
 */
void flightDetailShow(const services::adsb::Aircraft& plane);

/** True while the card is covering the radar. */
bool flightDetailVisible();

/** Drop the card. The caller is responsible for repainting the radar. */
void flightDetailHide();

}  // namespace ui
