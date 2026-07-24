#include "wled.h"

#ifdef BLACK
  #undef BLACK
#endif
#ifdef BLUE
  #undef BLUE
#endif
#ifdef GREEN
  #undef GREEN
#endif
#ifdef RED
  #undef RED
#endif
#ifdef WHITE
  #undef WHITE
#endif
#ifdef YELLOW
  #undef YELLOW
#endif
#ifdef CYAN
  #undef CYAN
#endif
#ifdef MAGENTA
  #undef MAGENTA
#endif
#ifdef PURPLE
  #undef PURPLE
#endif
#ifdef ORANGE
  #undef ORANGE
#endif
#ifdef DARKGREY
  #undef DARKGREY
#endif

#include <Arduino_GFX_Library.h>

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    Arduino_DataBus* bus = nullptr;
    Arduino_GFX* gfx = nullptr;
    
    uint16_t blocksW = 0;
    uint16_t blocksH = 0;
    uint16_t canvasW = 0;
    uint16_t canvasH = 0;
    
    bool initDone = false;
    bool pinsAllocated = false;
    bool lastPowerState = true;

    bool checkSettings() {
      if (!strip.isMatrix) return false;
      
      uint16_t currentW = Segment::maxWidth;
      uint16_t currentH = Segment::maxHeight;
      
      if (currentW == 0 || currentH == 0) return false;
      
      if (currentW != blocksW || currentH != blocksH) {
        blocksW = currentW;
        blocksH = currentH;
        canvasW = gfx->width();
        canvasH = gfx->height();
        gfx->fillScreen(RGB565_BLACK);
      }
      return true;
    }

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Initializing clean settings..."));

      int8_t pinsToAllocate[] = {TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL};
      pinsAllocated = true;

      for (uint8_t i = 0; i < 6; i++) {
        if (pinsToAllocate[i] >= 0) {
          PinManager::allocatePin(pinsToAllocate[i], false, PinOwner::UM_Unspecified);
        }
      }

      pinMode(TFT_BL, OUTPUT);
      digitalWrite(TFT_BL, HIGH); 

      bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);

      gfx = new Arduino_ST7789(
        bus, TFT_RST, 1, true, 
        TFT_WIDTH, TFT_HEIGHT,
        35, 0, 35, 0
      );

      gfx->begin();
      gfx->fillScreen(RGB565_BLACK); 
      initDone = true;
    }

    void handleOverlayDraw() override {
      if (!initDone || !gfx || !lastPowerState) return;
      if (!checkSettings()) return;

      Segment& seg = strip.getSegment(0);
      gfx->startWrite();

      // BILINEAR INTERPOLATION HACK:
      // Loop over every physical pixel of the display panel.
      // We calculate smooth blending gradients between the low-res WLED pixels.
      for (uint16_t screenY = 0; screenY < canvasH; screenY += 2) { // Step by 2 for speed
        float virtualY = ((float)screenY / (float)canvasH) * (blocksH - 1);
        uint16_t y0 = (uint16_t)virtualY;
        uint16_t y1 = (y0 < blocksH - 1) ? y0 + 1 : y0;
        float dy = virtualY - y0;

        for (uint16_t screenX = 0; screenX < canvasW; screenX += 2) { // Step by 2 for speed
          float virtualX = ((float)screenX / (float)canvasW) * (blocksW - 1);
          uint16_t x0 = (uint16_t)virtualX;
          uint16_t x1 = (x0 < blocksW - 1) ? x0 + 1 : x0;
          float dx = virtualX - x0;

          // Fetch the 4 surrounding low-res colors
          uint32_t c00 = seg.getPixelColorXY(x0, y0);
          uint32_t c10 = seg.getPixelColorXY(x1, y0);
          uint32_t c01 = seg.getPixelColorXY(x0, y1);
          uint32_t c11 = seg.getPixelColorXY(x1, y1);

          // Interpolate Red channel
          float r = ((c00 >> 16) & 0xFF) * (1 - dx) * (1 - dy) +
                    ((c10 >> 16) & 0xFF) * dx * (1 - dy) +
                    ((c01 >> 16) & 0xFF) * (1 - dx) * dy +
                    ((c11 >> 16) & 0xFF) * dx * dy;

          // Interpolate Green channel
          float g = ((c00 >> 8) & 0xFF) * (1 - dx) * (1 - dy) +
                    ((c10 >> 8) & 0xFF) * dx * (1 - dy) +
                    ((c01 >> 8) & 0xFF) * (1 - dx) * dy +
                    ((c11 >> 8) & 0xFF) * dx * dy;

          // Interpolate Blue channel
          float b = (c00 & 0xFF) * (1 - dx) * (1 - dy) +
                    (c10 & 0xFF) * dx * (1 - dy) +
                    (c01 & 0xFF) * (1 - dx) * dy +
                    (c11 & 0xFF) * dx * dy;

          uint16_t color16 = gfx->color565((uint8_t)r, (uint8_t)g, (uint8_t)b);
          
          // Draw a small 2x2 micro-block to instantly double the rendering speed
          gfx->writeFillRect(screenX, screenY, 2, 2, color16);
        }
      }
      
      gfx->endWrite();
    }

    void loop() override {
      if (!initDone || !gfx) return;

      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        lastPowerState = currentPowerState;
        digitalWrite(TFT_BL, lastPowerState ? HIGH : LOW);
        if (!lastPowerState) {
          gfx->fillScreen(RGB565_BLACK);
        }
      }
    }

    void addToJsonState(JsonObject& root) override {
      JsonObject top = root.createNestedObject("TTGO_Display");
      top["active"] = initDone;
    }

    uint16_t getId() override {
      return USERMOD_ID_TTGO_TDISPLAY_OUTPUT;
    }
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);
