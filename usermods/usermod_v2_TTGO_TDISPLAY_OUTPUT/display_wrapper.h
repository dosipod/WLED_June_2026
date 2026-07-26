#ifndef DISPLAY_WRAPPER_H
#define DISPLAY_WRAPPER_H

#include "wled.h"

#if defined(USER_SETUP_LOADED) || defined(TFT_CS)
  #include "lcd_as_output_espi_engine.h"
  #define IS_ESPI_ACTIVE 1
#else
  #include "lcd_as_output_gfx_engine.h"
  #define IS_ESPI_ACTIVE 0
#endif

class DisplayWrapper {
  private:
    #if (IS_ESPI_ACTIVE == 1)
      LcdTfteSpiEngine driver;
    #else
      LcdGfxEngine driver;
    #endif

  public:
    void initDriver(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst, uint16_t w, uint16_t h, int16_t colOffset) {
      #if (IS_ESPI_ACTIVE == 1)
        driver.init();
      #else
        driver.init(mosi, sclk, cs, dc, rst, w, h, colOffset);
      #endif
    }

    void clearScreen() {
      driver.clear();
    }

    void renderFrame(WS2812FX& strip) {
      driver.drawFrame(strip);
    }
};

#endif
