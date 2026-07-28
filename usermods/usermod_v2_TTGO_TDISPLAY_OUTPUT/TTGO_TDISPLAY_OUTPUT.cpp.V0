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

#if __has_include(<Arduino_GFX_Library.h>)
#include <Arduino_GFX_Library.h>

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    Arduino_DataBus* bus = nullptr;
    Arduino_GFX* gfx = nullptr;
    
    uint16_t blocksW = 0;
    uint16_t blocksH = 0;
    
    // NATIVE INI RESOLUTION BINDINGS:
    // Read the landscape matrix bounds using the flipped parameters from your file configuration maps
    const uint16_t targetWidth = (uint16_t)TFT_HEIGHT; // 320
    const uint16_t targetHeight = (uint16_t)TFT_WIDTH; // 170
    
    uint16_t blockWidth = 1;
    uint16_t blockHeight = 1;
    
    bool initDone = false;
    bool lastPowerState = true;

    void initializeDisplay() {
      if (initDone) return;

      // Extract your native environment pin definitions
      int8_t pinMosi = (int8_t)TFT_MOSI;
      int8_t pinSclk = (int8_t)TFT_SCLK;
      int8_t pinCs   = (int8_t)TFT_CS;
      int8_t pinDc   = (int8_t)TFT_DC;
      int8_t pinRst  = (int8_t)TFT_RST;
      int8_t pinBl   = (int8_t)TFT_BL;

      PinManager::allocatePin(pinMosi, false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinSclk, false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinCs,   false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinDc,   false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinRst,  false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinBl,   false, PinOwner::UM_Unspecified);

      pinMode(pinBl, OUTPUT);
      digitalWrite(pinBl, HIGH);

      bus = new Arduino_ESP32SPI(pinDc, pinCs, pinSclk, pinMosi, -1);

      // AUTOMATED OFFSET MAPPING BOUNDS:
      // Evaluates your active -D CGRAM_OFFSET flag from your ini file. If active, it 
      // automatically shifts display tracking by 35px to clear screen alignment errors.
      #ifdef CGRAM_OFFSET
        int16_t currentOffset = 35;
      #else
        int16_t currentOffset = 0;
      #endif

      // Strict INI-Driven Driver Allocation footprint map
      gfx = new Arduino_ST7789(
        bus, 
        pinRst, 
        0,                  // initial rotation tracking parameter
        true,               // IPS panel flag
        (int16_t)TFT_WIDTH,  // 170 (Native portrait panel physical thickness boundary)
        (int16_t)TFT_HEIGHT, // 320 (Native portrait panel physical row depth boundary)
        currentOffset,      // col_offset1 mapping coordinate
        0,                  // row_offset1 mapping coordinate
        currentOffset,      // col_offset2 mapping coordinate
        0                   // row_offset2 mapping coordinate
      );
      
      if (gfx) {
        gfx->begin();
        gfx->setRotation(1); // Force landscape transformation loop matching targetWidth/targetHeight
        gfx->fillScreen(RGB565_BLACK);
        initDone = true;
      }
    }

  public:
    void setup() override {
      // Clear of blocking hooks
    }

    void handleOverlayDraw() override {
      if (!initDone) {
        initializeDisplay();
      }

      if (!initDone || !lastPowerState || !gfx) return;

      Segment& seg = strip.getSegment(0);
      uint16_t segW = seg.width();
      uint16_t segH = seg.height();
      
      if (segW == 0 || segH == 0) return;

      if (segW != blocksW || segH != blocksH) {
        blocksW = segW;
        blocksH = segH;
        
        // Calculate block bounds cleanly against your native landscape targets
        blockWidth  = targetWidth / blocksW;
        blockHeight = targetHeight / blocksH;
        
        if (blockWidth == 0)  blockWidth  = 1;
        if (blockHeight == 0) blockHeight = 1;
        
        gfx->fillScreen(RGB565_BLACK);
      }

      gfx->startWrite();
      for (int h = 0; h < blocksH; h++) {
        uint16_t py = h * blockHeight;
        for (int w = 0; w < blocksW; w++) {
          uint32_t c = strip.getPixelColor(w + (h * blocksW));
          uint16_t color16 = gfx->color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
          
          gfx->writeFillRect(w * blockWidth, py, blockWidth, blockHeight, color16);
        }
      }
      gfx->endWrite();
    }

    void loop() override {
      if (!initDone || !gfx) return;

      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        lastPowerState = currentPowerState;
        int8_t pinBl = (int8_t)TFT_BL;
        digitalWrite(pinBl, lastPowerState ? HIGH : LOW);
        if (!lastPowerState) {
          gfx->fillScreen(RGB565_BLACK);
        }
      }
    }

    uint16_t getId() override {
      return USERMOD_ID_UNSPECIFIED;
    }

    ~TTGO_TDISPLAY_OUTPUT() {
      if (gfx) delete gfx;
      if (bus) delete bus;
    }
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);

#endif
