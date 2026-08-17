#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

/**
 * LovyanGFX device. Panel type and bus follow the board selected in config.h;
 * every pin value comes from there.
 */
class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_SPI _bus;

#if defined(BOARD_CYD)
#if defined(CYD_PANEL_ST7789)
  lgfx::Panel_ST7789 _panel;
#else
  lgfx::Panel_ILI9341 _panel;
#endif
  lgfx::Light_PWM _light;
  lgfx::Touch_XPT2046 _touch;
#else
  lgfx::Panel_GC9A01 _panel;
#endif

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.freq_write = config::kDisplaySpiWriteHz;
      cfg.pin_sclk = static_cast<int>(config::kDisplayPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kDisplayPinMosi);
      cfg.pin_miso = -1;
      cfg.pin_dc = static_cast<int>(config::kDisplayPinDc);
#if defined(BOARD_CYD)
      // Classic ESP32: DMA keeps the full-frame blit off the CPU.
      cfg.spi_mode = 0;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
#endif
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = static_cast<int>(config::kDisplayPinCs);
      cfg.pin_rst = static_cast<int>(config::kDisplayPinRst);
      cfg.invert = config::kDisplayInvert;
      cfg.rgb_order = config::kDisplayRgbOrder;
#if defined(BOARD_CYD)
      // The glass is native 240×320 portrait; setRotation() turns it landscape.
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.memory_width = 240;
      cfg.memory_height = 320;
#endif
      _panel.config(cfg);
    }
#if defined(BOARD_CYD)
    {
      auto cfg = _light.config();
      cfg.pin_bl = static_cast<int>(config::kDisplayPinBacklight);
      cfg.invert = false;  // active HIGH
      cfg.freq = 12000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    {
      // The touch controller sits on separate pins from the panel, so it gets
      // the second SPI host rather than sharing the display bus.
      auto cfg = _touch.config();
      cfg.x_min = config::kTouchXMin;
      cfg.x_max = config::kTouchXMax;
      cfg.y_min = config::kTouchYMin;
      cfg.y_max = config::kTouchYMax;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.spi_host = SPI3_HOST;
      cfg.freq = 1000000;
      cfg.pin_sclk = static_cast<int>(config::kTouchPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kTouchPinMosi);
      cfg.pin_miso = static_cast<int>(config::kTouchPinMiso);
      cfg.pin_cs = static_cast<int>(config::kTouchPinCs);
      cfg.pin_int = static_cast<int>(config::kTouchPinIrq);
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
#endif
    setPanel(&_panel);
  }
};
