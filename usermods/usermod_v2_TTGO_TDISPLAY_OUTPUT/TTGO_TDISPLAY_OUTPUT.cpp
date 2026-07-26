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
    
    // VERIFIED DYNAMIC ENVIRONMENT ASSIGNMENTS:
    // Pulls constraints directly out of your platformio.ini environment settings if declared,
    // otherwise defaults back safely to the pristine 1.9" panel metrics.
    #ifdef TFT_WIDTH
      const uint16_t targetWidth = (uint16_t)TFT_WIDTH;
    #else
      const uint16_t targetWidth = 320;
    #endif

    #ifdef TFT_HEIGHT
      const uint16_t targetHeight = (uint16_t)TFT_HEIGHT;
    #else
      const uint16_t targetHeight = 170;
    #endif

    #ifdef TFT_OFFSET
      const int16_t fallbackOffset = (int16_t)TFT_OFFSET;
    #else
      const int16_t fallbackOffset = 35;
    #endif
    
    uint16_t blockWidth = 1;
    uint16_t blockHeight = 1;
    
    bool initDone = false;
    bool lastPowerState = true;

    void initializeDisplay() {
      if (initDone) return;

      // Dynamic pin mapping fallback parameters
      #if defined(TFT_MOSI) && defined(TFT_SCLK) && defined(TFT_CS) && defined(TFT_DC) && defined(TFT_RST) && defined(TFT_BL)
        int8_t pinMosi = (int8_t)TFT_MOSI;
        int8_t pinSclk = (int8_t)TFT_SCLK;
        int8_t pinCs   = (int8_t)TFT_CS;
        int8_t pinDc   = (int8_t)TFT_DC;
        int8_t pinRst  = (int8_t)TFT_RST;
        int8_t pinBl   = (int8_t)TFT_BL;
      #else
        int8_t pinMosi = 13;
        int8_t pinSclk = 12;
        int8_t pinCs   = 10;
        int8_t pinDc   = 11;
        int8_t pinRst  = 1;
        int8_t pinBl   = 14;
      #endif

      PinManager::allocatePin(pinMosi, false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinSclk, false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinCs,   false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinDc,   false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinRst,  false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinBl,   false, PinOwner::UM_Unspecified);

      pinMode(pinBl, OUTPUT);
      digitalWrite(pinBl, HIGH);

      bus = new Arduino_ESP32SPI(pinDc, pinCs, pinSclk, pinMosi, -1);

      // UNIFIED CROSS-PLATFORM CONSTRUCTOR:
      // Dynamically flips or registers row/column offsets to look at your environment parameters.
      // To correctly map a standard TTGO panel (which rotates opposite to the 1.9"), we compute 
      // the portrait matrix layout dynamically based on targeted resolution boundaries.
      uint16_t constructorWidth  = (targetWidth > targetHeight) ? targetHeight : targetWidth;
      uint16_t constructorHeight = (targetWidth > targetHeight) ? targetWidth  : targetHeight;

      gfx = new Arduino_ST7789(
        bus, 
        pinRst, 
        0,                 // initial rotation tracking parameter
        true,              // IPS panel flag
        constructorWidth,  // physical native panel column width
        constructorHeight, // physical native panel row height
        fallbackOffset,    // col_offset1 mapping coordinate
        0,                 // row_offset1 mapping coordinate
        fallbackOffset,    // col_offset2 mapping coordinate
        0                  // row_offset2 mapping coordinate
      );
      
      if (gfx) {
        gfx->begin();
        
        // Match rotation configurations based on macro presence strings
        #ifdef TFT_ROTATION
          gfx->setRotation(TFT_ROTATION);
        #else
          gfx->setRotation(1); 
        #endif
        
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
        
        #ifdef TFT_BL
          int8_t pinBl = (int8_t)TFT_BL;
        #else
          int8_t pinBl = 14;
        #endif
        
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
