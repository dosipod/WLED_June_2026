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

      // Reverted to full 170x320 canvas configuration with 0 offsets to clear the static margins
      gfx = new Arduino_ST7789(
        bus, 
        TFT_RST, 
        1,     // Landscape orientation 
        true,  // IPS mode
        170,   // Panel native height
        320,   // Panel native width
        0,     // 0 offset ensures the canvas fills the entire RAM window
        0      
      );

      gfx->begin();
      gfx->fillScreen(RGB565_BLACK); 
      
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Setup Complete. Canvas cleared."));
      initDone = true;
    }

    // Intercepting the core frame output rather than using handleOverlayDraw
    void show() override {
      if (!initDone || !gfx || !lastPowerState) return;

      // Access the live matrix layout configurations
      uint16_t width = strip.isMatrix ? segments[0].width() : 320;
      uint16_t height = strip.isMatrix ? segments[0].height() : 1;
      
      gfx->startWrite();
      
      // Dual-axis mapping to safely scale WLED matrix data to your screen aspect ratio
      for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
          // Fetch color data directly from the active engine layer
          uint32_t c = strip.isMatrix ? strip.getPixelColor(xy32(x, y)) : strip.getPixelColor(x);
          
          uint8_t r = (c >> 16) & 0xFF;
          uint8_t g = (c >> 8)  & 0xFF;
          uint8_t b = c         & 0xFF;
          
          uint16_t color16 = gfx->color565(r, g, b);
          
          // Render pixels, scaling up the output dimensions if you are running a small virtual matrix size
          if (strip.isMatrix) {
            gfx->writeFillRect(x * 4, y * 4, 4, 4, color16);
          } else {
            gfx->writePixel(x % 320, y, color16);
          }
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

      // Overlay text data on top of the running canvas sequence
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
