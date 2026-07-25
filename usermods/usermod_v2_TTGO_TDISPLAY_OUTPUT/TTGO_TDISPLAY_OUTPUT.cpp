#ifndef LCD_AS_OUTPUT_GFX_ENGINE_H
#define LCD_AS_OUTPUT_GFX_ENGINE_H

#if __has_include(<Arduino_GFX_Library.h>)
#include <Arduino_GFX_Library.h>

class LcdGfxEngine {
  private:
    Arduino_DataBus* bus = nullptr;
    Arduino_GFX* gfx = nullptr;
    uint16_t blocksW = 0, blocksH = 0;
    uint16_t canvasW = 0, canvasH = 0;
    uint16_t blockWidth = 1, blockHeight = 1;
    bool isInit = false;

  public:
    void init(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst, uint16_t w, uint16_t h, int16_t colOffset) {
      if (gfx) { delete gfx; gfx = nullptr; }
      if (bus) { delete bus; bus = nullptr; }
      isInit = false;

      if (mosi < 0 || sclk < 0 || cs < 0 || dc < 0 || rst < 0) return;

      bus = new Arduino_ESP32SPI(dc, cs, sclk, mosi, -1);
      gfx = new Arduino_ST7789(bus, rst, 1, true, h, w, colOffset, 0, colOffset, 0);
      gfx->begin();
      gfx->fillScreen(RGB565_BLACK);
      isInit = true;
    }

    void clear() {
      if (isInit && gfx) gfx->fillScreen(RGB565_BLACK);
    }

    void drawFrame(WS2812FX& strip) {
      if (!isInit || !gfx || !strip.isMatrix) return;
      
      Segment& seg = strip.getSegment(0);
      uint16_t segW = seg.width();
      uint16_t segH = seg.height();
      if (segW == 0 || segH == 0) return;

      if (segW != blocksW || segH != blocksH) {
        blocksW = segW;
        blocksH = segH;
        canvasW = gfx->width();
        canvasH = gfx->height();
        blockWidth  = canvasW / blocksW;
        blockHeight = canvasH / blocksH;
        if (blockWidth == 0)  blockWidth  = 1;
        if (blockHeight == 0) blockHeight = 1;
        gfx->fillScreen(RGB565_BLACK);
      }

      gfx->startWrite();
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
#else
class LcdGfxEngine {
  public:
    void init(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst, uint16_t w, uint16_t h, int16_t colOffset) {}
    void clear() {}
    void drawFrame(WS2812FX& strip) {}
};
#endif

#endif
