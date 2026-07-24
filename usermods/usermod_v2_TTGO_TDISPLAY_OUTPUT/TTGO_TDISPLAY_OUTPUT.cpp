#include "wled.h"

// Clear 32-bit naming conflicts to keep compilation fast and warning-free
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
    
    // Caching layout matrices to remove loop-level division hooks
    uint16_t blocksW = 0;
    uint16_t blocksH = 0;
    uint16_t canvasW = 0;
    uint16_t canvasH = 0;
    
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    
    bool initDone = false;
    bool pinsAllocated = false;
    bool lastPowerState = true;
    
    // High-speed scanline cache buffer to hold 320 raw 16-bit RGB pixels
    uint16_t scanlineBuffer[320];

    bool checkSettings() {
      if (!strip.isMatrix) return false;
      
      uint16_t currentW = Segment::maxWidth;
      uint16_t currentH = Segment::maxHeight;
      
      if (currentW == 0 || currentH == 0) return false;
      
      // Only recalculate scales if the layout configuration modifications update
      if (currentW != blocksW || currentH != blocksH) {
        blocksW = currentW;
        blocksH = currentH;
        
        canvasW = gfx->width();
        canvasH = gfx->height();
        
        scaleX = (float)canvasW / (float)blocksW;
        scaleY = (float)canvasH / (float)blocksH;
        
        gfx->fillScreen(RGB565_BLACK);
      }
      return true;
    }

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Initializing high-FPS display pipelines..."));

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
        170, 320, 35, 0, 35, 0
      );

      gfx->begin();
      gfx->fillScreen(RGB565_BLACK); 
      
      initDone = true;
    }

    void handleOverlayDraw() override {
      if (!initDone || !gfx || !lastPowerState) return;
      if (!checkSettings()) return;

      Segment& seg = strip.getSegment(0);

      // Start hardware stream operation
      gfx->startWrite();
      
      // Loop over every physical display line (170 rows)
      for (uint16_t screenY = 0; screenY < canvasH; screenY++) {
        
        // Reverse-map physical display Y coordinate to virtual matrix coordinate
        uint16_t virtualY = (uint16_t)((float)screenY / scaleY);
        if (virtualY >= blocksH) virtualY = blocksH - 1;
        
        uint16_t lastVirtualX = 9999;
        uint16_t cachedColor16 = 0;

        // Populate the rapid scanline array buffer for the entire physical display row (320 cols)
        for (uint16_t screenX = 0; screenX < canvasW; screenX++) {
          
          uint16_t virtualX = (uint16_t)((float)screenX / scaleX);
          if (virtualX >= blocksW) virtualX = blocksW - 1;
          
          // Optimization: Only grab and convert color parameters if the virtual coordinate changed
          if (virtualX != lastVirtualX) {
            lastVirtualX = virtualX;
            uint32_t c = seg.getPixelColorXY(virtualX, virtualY);
            cachedColor16 = gfx->color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
          }
          
          scanlineBuffer[screenX] = cachedColor16;
        }
        
        // Push the entire populated row to the display module at hardware speeds
        gfx->draw16bitRGBBitmap(0, screenY, scanlineBuffer, canvasW, 1);
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
