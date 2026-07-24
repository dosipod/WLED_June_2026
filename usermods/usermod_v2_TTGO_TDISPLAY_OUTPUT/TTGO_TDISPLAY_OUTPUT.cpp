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
    
    // Fixed-point scaling increments (Shifted by 16 bits)
    uint32_t stepX_fp = 0;
    uint32_t stepY_fp = 0;
    
    bool initDone = false;
    bool pinsAllocated = false;
    bool lastPowerState = true;
    
    uint16_t* scanlineBuffer = nullptr;

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
        
        // Calculate the precise matrix-to-canvas coordinate steps inside the configuration check
        stepX_fp = ((uint32_t)blocksW << 16) / canvasW;
        stepY_fp = ((uint32_t)blocksH << 16) / canvasH;
        
        if (scanlineBuffer) delete[] scanlineBuffer;
        scanlineBuffer = new uint16_t[canvasW];
        
        gfx->fillScreen(RGB565_BLACK);
      }
      return true;
    }

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Initializing portable dynamic settings..."));

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

      // Inherits parameters directly from your platformio.ini target setup variables
      gfx = new Arduino_ST7789(
        bus, 
        TFT_RST, 
        1,         // Landscape orientation mapping
        true,      // IPS mode active
        TFT_WIDTH, 
        TFT_HEIGHT,
        35,        // Offset settings matching the panel specifications
        0,
        35,
        0
      );

      gfx->begin();
      gfx->fillScreen(RGB565_BLACK); 
      initDone = true;
    }

    void handleOverlayDraw() override {
      if (!initDone || !gfx || !lastPowerState || !scanlineBuffer) return;
      if (!checkSettings()) return;

      Segment& seg = strip.getSegment(0);
      gfx->startWrite();
      
      uint32_t accumY_fp = 0;

      for (uint16_t screenY = 0; screenY < canvasH; screenY++) {
        // Derive virtual matrix row index purely through bit-shifting addition increments
        uint16_t virtualY = accumY_fp >> 16;
        if (virtualY >= blocksH) virtualY = blocksH - 1;
        accumY_fp += stepY_fp;
        
        uint16_t lastVirtualX = 9999;
        uint16_t cachedColor16 = 0;
        uint32_t accumX_fp = 0;

        for (uint16_t screenX = 0; screenX < canvasW; screenX++) {
          uint16_t virtualX = accumX_fp >> 16;
          if (virtualX >= blocksW) virtualX = blocksW - 1;
          accumX_fp += stepX_fp;
          
          if (virtualX != lastVirtualX) {
            lastVirtualX = virtualX;
            uint32_t c = seg.getPixelColorXY(virtualX, virtualY);
            cachedColor16 = gfx->color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
          }
          
          scanlineBuffer[screenX] = cachedColor16;
        }
        
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

    ~TTGO_TDISPLAY_OUTPUT() {
      if (scanlineBuffer) delete[] scanlineBuffer;
    }
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);
