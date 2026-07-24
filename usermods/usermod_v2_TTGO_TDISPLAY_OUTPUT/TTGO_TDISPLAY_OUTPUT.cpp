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
    
    // Dynamic Downscale parameters bound to the WLED UI sliders
    int sampleW = 53;  
    int sampleH = 27;  
    
    uint16_t blockWidth = 1;
    uint16_t blockHeight = 1;
    
    bool initDone = false;
    bool pinsAllocated = false;
    bool lastPowerState = true;

    // Fixed sizes explicitly declared to resolve flexible member errors
    const char SETTING_SAMPLE_W[13] = "Sample-Width";
    const char SETTING_SAMPLE_H[14] = "Sample-Height";

    bool checkSettings() {
      if (!strip.isMatrix) return false;
      
      uint16_t currentW = Segment::maxWidth;
      uint16_t currentH = Segment::maxHeight;
      
      if (currentW == 0 || currentH == 0) return false;
      
      if (sampleW < 4)   sampleW = 4;
      if (sampleW > 320) sampleW = 320;
      if (sampleH < 4)   sampleH = 4;
      if (sampleH > 170) sampleH = 170;
      
      if (currentW != blocksW || currentH != blocksH) {
        blocksW = currentW;
        blocksH = currentH;
        
        canvasW = gfx->width();
        canvasH = gfx->height();
        
        blockWidth  = canvasW / (uint16_t)sampleW;
        blockHeight = canvasH / (uint16_t)sampleH;
        
        if (blockWidth == 0)  blockWidth  = 1;
        if (blockHeight == 0) blockHeight = 1;
        
        gfx->fillScreen(RGB565_BLACK);
      }
      return true;
    }

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Starting Web-Configured Sub-Sampler..."));

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

      uint16_t currentBlockWidth  = canvasW / (uint16_t)sampleW;
      uint16_t currentBlockHeight = canvasH / (uint16_t)sampleH;
      if (currentBlockWidth == 0)  currentBlockWidth  = 1;
      if (currentBlockHeight == 0) currentBlockHeight = 1;

      for (uint16_t y = 0; y < (uint16_t)sampleH; y++) {
        uint16_t py = y * currentBlockHeight;
        uint16_t virtualY = (y * blocksH) / (uint16_t)sampleH;
        if (virtualY >= blocksH) virtualY = blocksH - 1;
        
        for (uint16_t x = 0; x < (uint16_t)sampleW; x++) {
          uint16_t virtualX = (x * blocksW) / (uint16_t)sampleW;
          if (virtualX >= blocksW) virtualX = blocksW - 1;
          
          uint32_t c = seg.getPixelColorXY(virtualX, virtualY);
          uint16_t color16 = gfx->color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
          uint16_t px = x * currentBlockWidth;
          
          gfx->writeFillRect(px, py, currentBlockWidth, currentBlockHeight, color16);
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

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(F("DisplayMatrix"));
      top[FPSTR(SETTING_SAMPLE_W)] = sampleW;
      top[FPSTR(SETTING_SAMPLE_H)] = sampleH;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[F("DisplayMatrix")];
      if (top.isNull()) return false;

      bool configChanged = false;
      int oldW = sampleW;
      int oldH = sampleH;

      // Typo corrected: closing brackets aligned properly outside the FPSTR macros
      if (top[FPSTR(SETTING_SAMPLE_W)].is<int>()) sampleW = top[FPSTR(SETTING_SAMPLE_W)];
      if (top[FPSTR(SETTING_SAMPLE_H)].is<int>()) sampleH = top[FPSTR(SETTING_SAMPLE_H)];

      if (sampleW != oldW || sampleH != oldH) {
        blocksW = 0; 
      }
      return true;
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
