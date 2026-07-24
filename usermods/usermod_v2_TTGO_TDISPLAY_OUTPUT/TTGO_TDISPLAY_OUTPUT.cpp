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
    unsigned long lastUpdate = 0;
    bool initDone = false;
    bool pinsAllocated = false;
    bool lastPowerState = true;

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Starting clean setup..."));

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

      // Match the physical landscape size and hardware resolution mapping exactly
      gfx = new Arduino_ST7789(
        bus, 
        TFT_RST, 
        1,     // Landscape
        true,  // IPS mode
        320,   // Canvas Width
        170,   // Canvas Height
        0,     // 0 offset to remove borders
        0      
      );

      gfx->begin();
      gfx->fillScreen(RGB565_BLACK); 
      
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Setup Complete. Canvas cleared."));
      initDone = true;
    }

    void handleOverlayDraw() override {
      if (!initDone || !gfx || !lastPowerState) return;

      // Extract virtual screen matrix dimensions using the custom branch core configuration variables
      uint16_t matrixWidth  = strip.isMatrix ? Segment::maxWidth  : 1;
      uint16_t matrixHeight = strip.isMatrix ? Segment::maxHeight : 1;

      if (matrixWidth == 0 || matrixHeight == 0) return;

      gfx->startWrite();
      
      // Dual-axis 2D Matrix coordinate mapping loops
      for (uint16_t y = 0; y < matrixHeight; y++) {
        for (uint16_t x = 0; x < matrixWidth; x++) {
          
          // Fetch pixel calculation index natively mapped to the performance bus array
          uint32_t c = strip.getPixelColor(x + (y * matrixWidth));
          
          uint8_t r = (c >> 16) & 0xFF;
          uint8_t g = (c >> 8)  & 0xFF;
          uint8_t b = c         & 0xFF;
          
          uint16_t color16 = gfx->color565(r, g, b);
          
          // Scale blocks dynamically to scale the display array directly up to the 320x170 glass
          uint16_t blockWidth  = 320 / matrixWidth;
          uint16_t blockHeight = 170 / matrixHeight;
          if (blockWidth == 0)  blockWidth  = 1;
          if (blockHeight == 0) blockHeight = 1;

          gfx->writeFillRect(x * blockWidth, y * blockHeight, blockWidth, blockHeight, color16);
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

      if (!lastPowerState) return;

      if (millis() - lastUpdate > 5000) {
        lastUpdate = millis();
        gfx->setTextSize(2);
        gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
        gfx->setCursor(10, 10);
        gfx->println(WiFi.localIP().toString().c_str());
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
