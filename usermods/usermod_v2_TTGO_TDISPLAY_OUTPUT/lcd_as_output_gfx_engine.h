#ifndef LCD_AS_OUTPUT_GFX_ENGINE_H
#define LCD_AS_OUTPUT_GFX_ENGINE_H

#include <Arduino_GFX_Library.h>

class LcdGfxEngine {
  private:
    Arduino_DataBus* bus = nullptr;
    Arduino_GFX* gfx = nullptr;
    uint16_t blocksW = 0;
    uint16_t blocksH = 0;
    uint16_t canvasW = 0;
    uint16_t canvasH = 0;
    uint16_t blockWidth = 1;
    uint16_t blockHeight = 1;

  public:
    void init(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst, uint16_t w, uint16_t h) {
      bus = new Arduino_ESP32SPI(dc, cs, sclk, mosi, -1);
      
      // Official 170x320 panel footprint mapping with dual-axis landscape offsets
      gfx = new Arduino_ST7789(bus, rst, 1, true, w, h, 35, 0, 35, 0);
      gfx->begin();
      gfx->fillScreen(RGB565_BLACK);
    }

    void clear() {
      if (gfx) gfx->fillScreen(RGB565_BLACK);
    }

    void drawFrame(WS2812FX& strip) {
      if (!gfx || !strip.isMatrix) return;
      
      Segment& seg = strip.getSegment(0);
      uint16_t currentW = seg.width();
      uint16_t currentH = seg.height();
      if (currentW == 0 || currentH == 0) return;

      // Handle structural canvas adjustments adaptively
      if (currentW != blocksW || currentH != blocksH) {
        blocksW = currentW;
        blocksH = currentH;
        canvasW = gfx->width();
        canvasH = gfx->height();
        blockWidth  = canvasW / blocksW;
        blockHeight = canvasH / blocksH;
        if (blockWidth == 0)  blockWidth  = 1;
        if (blockHeight == 0) blockHeight = 1;
        gfx->fillScreen(RGB565_BLACK);
      }

      gfx->startWrite();
      // Render frame matrix using your classic, reliable segment calculation loops
      for (int h = 0; h < blocksH; h++) {
        uint16_t py = h * blockHeight;
        for (int w = 0; w < blocksW; w++) {
          uint32_t c = strip.getPixelColor(seg.start + w + h * seg.width());
          uint16_t color16 = gfx->color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
          gfx->writeFillRect(w * blockWidth, py, blockWidth, blockHeight, color16);
        }
      }
      gfx->endWrite();
    }
};

#endif
