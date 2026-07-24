#ifndef LCD_AS_OUTPUT_ESPI_ENGINE_H
#define LCD_AS_OUTPUT_ESPI_ENGINE_H

#include <TFT_eSPI.h>

class LcdTfteSpiEngine {
  private:
    TFT_eSPI tft = TFT_eSPI();
    uint16_t blocksW = 0, blocksH = 0;
    uint16_t blockWidth = 1, blockHeight = 1;
    bool isInit = false;

  public:
    void init() {
      tft.init();
      tft.setRotation(1);
      tft.fillScreen(TFT_BLACK);
      isInit = true;
    }

    void clear() {
      if (isInit) tft.fillScreen(TFT_BLACK);
    }

    void drawFrame(WS2812FX& strip) {
      if (!isInit || !strip.isMatrix) return;
      
      Segment& seg = strip.getSegment(0);
      if (seg.width() != blocksW || seg.height() != blocksH) {
        blocksW = seg.width();
        blocksH = seg.height();
        blockWidth  = tft.width() / blocksW;
        blockHeight = tft.height() / blocksH;
        tft.fillScreen(TFT_BLACK);
      }

      for (int h = 0; h < blocksH; h++) {
        for (int w = 0; w < blocksW; w++) {
          uint32_t c = strip.getPixelColor(seg.start + w + h * seg.width());
          tft.fillRect(w * blockWidth, h * blockHeight, blockWidth, blockHeight, tft.color24to16(c));
        }
      }
    }
};

#endif
