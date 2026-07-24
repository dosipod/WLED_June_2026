#include "wled.h"

// 1. Cleanly isolate color macro conflicts before loading the GFX engine
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

      // 2. Corrected Dimensions: Explicitly targeting standard 135x240 S3 panel offsets
      gfx = new Arduino_ST7789(
        bus, 
        TFT_RST, 
        1,     // Landscape
        true,  // IPS
        135,   // True Screen Width
        240,   // True Screen Height
        52,    // Accurate X offset for 135x240 displays
        40     // Accurate Y offset for 135x240 displays
      );

      gfx->begin();
      gfx->fillScreen(RGB565_BLACK); // Explicit safe 16-bit call
      
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Setup Complete. No macro warnings."));
      initDone = true;
    }

    // 3. Frame Update: Render live effects straight to the panel
    void handleOverlayDraw() override {
      if (!initDone || !gfx || !lastPowerState) return;

      uint16_t totalPixels = strip.getLengthTotal();
      if (totalPixels == 0) return;

      // Start raw data frame burst to draw pixel streams instantly
      gfx->startWrite();
      
      for (uint16_t i = 0; i < totalPixels; i++) {
        uint32_t c = strip.getPixelColor(i);
        
        // Use native WLED bit-shifting wrappers safely
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8)  & 0xFF;
        uint8_t b = c         & 0xFF;
        
        uint16_t color16 = gfx->color565(r, g, b);
        
        // Automatic layout distribution mapping directly into 240 pixel limits
        gfx->writePixel(i % 240, i / 240, color16);
      }
      
      gfx->endWrite();
    }

    void loop() override {
      if (!initDone || !gfx) return;

      // 4. UI Master Power Binding: Sync display backlight to the WLED power button
      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        lastPowerState = currentPowerState;
        digitalWrite(TFT_BL, lastPowerState ? HIGH : LOW);
        if (!lastPowerState) {
          gfx->fillScreen(RGB565_BLACK);
        }
      }

      if (!lastPowerState) return;

      // Draw active IP over the top layer every 4 seconds
      if (millis() - lastUpdate > 4000) {
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
