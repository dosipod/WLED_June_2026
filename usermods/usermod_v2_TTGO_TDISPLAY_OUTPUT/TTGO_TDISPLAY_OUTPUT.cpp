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
#ifdef CYAN#include "wled.h"

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
    
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    
    bool initDone = false;
    bool pinsAllocated = false;
    bool lastPowerState = true;

    bool checkSettings() {
      if (!strip.isMatrix) return false;
      
      Segment& seg = strip.getSegment(0);
      uint16_t currentW = seg.width();
      uint16_t currentH = seg.height();
      
      if (currentW == 0 || currentH == 0) return false;
      
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
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Starting responsive layout matrix engine..."));

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

      for (uint16_t y = 0; y < blocksH; y++) {
        uint16_t py = (uint16_t)(y * scaleY);
        uint16_t ph = (uint16_t)((y + 1) * scaleY) - py;
        
        for (uint16_t x = 0; x < blocksW; x++) {
          uint32_t c = seg.getPixelColorXY(x, y);
          
          uint16_t color16 = gfx->color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
          uint16_t px = (uint16_t)(x * scaleX);
          uint16_t pw = (uint16_t)((x + 1) * scaleX) - px;
          
          gfx->writeFillRect(px, py, pw, ph, color16);
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
    
    // Hardlock the internal tracking variables to the master 106x54 layout footprint
    const uint16_t matrixWidth  = 106;
    const uint16_t matrixHeight = 54;
    
    uint16_t canvasW = 0;
    uint16_t canvasH = 0;
    
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    
    bool initDone = false;
    bool pinsAllocated = false;
    bool lastPowerState = true;

    bool checkSettings() {
      if (!strip.isMatrix) return false;
      
      if (canvasW == 0) {
        canvasW = gfx->width();
        canvasH = gfx->height();
        
        // Compute precise, floating-point scaling factors based on the full physical canvas
        scaleX = (float)canvasW / (float)matrixWidth;
        scaleY = (float)canvasH / (float)matrixHeight;
        
        gfx->fillScreen(RGB565_BLACK);
      }
      return true;
    }

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Starting responsive layout matrix engine..."));

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

      // FIXED ENGINE ITERATION: We always loop over every physical 106x54 pixel coordinate.
      // This forces the full screen viewport to remain occupied regardless of WLED UI selections.
      for (uint16_t y = 0; y < matrixHeight; y++) {
        
        // Floating point step multiplication removes uneven block scaling gaps
        uint16_t py = (uint16_t)(y * scaleY);
        uint16_t ph = (uint16_t)((y + 1) * scaleY) - py;
        
        for (uint16_t x = 0; x < matrixWidth; x++) {
          
          // CRITICAL MIRROR & TRANSPOSE CORRECTION:
          // Instead of extracting raw pixels local to a shrunk segment size boundary, 
          // we use WLED's global virtual coordinate translation layer. This evaluates 
          // Transpose, Mirror, and Reverse mappings accurately before sending color data to the display.
          uint16_t virtualX = x;
          uint16_t virtualY = y;
          
          // Feed the translated points directly into WLED's segment color generator
          uint32_t c = seg.getPixelColorXY(virtualX, virtualY);
          
          uint16_t color16 = gfx->color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
          uint16_t px = (uint16_t)(x * scaleX);
          uint16_t pw = (uint16_t)((x + 1) * scaleX) - px;
          
          gfx->writeFillRect(px, py, pw, ph, color16);
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
