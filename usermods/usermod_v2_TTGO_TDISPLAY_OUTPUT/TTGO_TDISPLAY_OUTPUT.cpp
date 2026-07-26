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
    uint16_t canvasW = 0;
    uint16_t canvasH = 0;
    uint16_t blockWidth = 1;
    uint16_t blockHeight = 1;
    
    bool initDone = false;
    bool lastPowerState = true;

    void initializeDisplay() {
      if (initDone) return;

      // Hardcoded pin allocations matching your ESP32-S3 panel configuration
      int8_t pinMosi = 13;
      int8_t pinSclk = 12;
      int8_t pinCs   = 10;
      int8_t pinDc   = 11;
      int8_t pinRst  = 1;
      int8_t pinBl   = 14;

      // Safe hardware registration with the WLED pin database
      PinManager::allocatePin(pinMosi, false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinSclk, false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinCs,   false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinDc,   false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinRst,  false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinBl,   false, PinOwner::UM_Unspecified);

      pinMode(pinBl, OUTPUT);
      digitalWrite(pinBl, HIGH);

      // Ultra-safe stack allocation 
      bus = new Arduino_ESP32SPI(pinDc, pinCs, pinSclk, pinMosi, -1);
      gfx = new Arduino_ST7789(bus, pinRst, 0 /* rotation */, true /* IPS */);
      
      if (gfx) {
        gfx->begin();
        gfx->setRotation(1); // Landscape view
        gfx->fillScreen(RGB565_BLACK);
        initDone = true;
      }
    }

  public:
    void setup() override {
      // Intentionally clear of any blocking operations to let WLED boot instantly
    }

    void handleOverlayDraw() override {
      // Lazy-load on the very first drawing frame tick
      if (!initDone) {
        initializeDisplay();
      }

      if (!initDone || !lastPowerState || !gfx) return;

      if (!strip.isMatrix) return;
      Segment& seg = strip.getSegment(0);
      uint16_t segW = seg.width();
      uint16_t segH = seg.height();
      
      // Strict out-of-bounds array protective buffer limits check
      int totalLedLength = strip.getLength();
      if (segW == 0 || segH == 0 || totalLedLength == 0) return;

      if (segW != blocksW || segH != blocksH) {
        blocksW = segW;
        blocksH = segH;
        canvasW = gfx->width();
        canvasH = gfx->height();
        
        // Accurate alignment math to stretch the frame edge-to-edge
        blockWidth  = (canvasW - 70) / blocksW;
        blockHeight = canvasH / blocksH;
        
        if (blockWidth == 0)  blockWidth  = 1;
        if (blockHeight == 0) blockHeight = 1;
        
        gfx->fillScreen(RGB565_BLACK);
      }

      gfx->startWrite();
      for (int h = 0; h < blocksH; h++) {
        uint16_t py = h * blockHeight;
        for (int w = 0; w < blocksW; w++) {
          int pixelIndex = seg.start + w + (h * segW);
          
          uint32_t c = 0;
          if (pixelIndex >= 0 && pixelIndex < totalLedLength) {
            c = strip.getPixelColor(pixelIndex);
          }
          
          uint16_t color16 = gfx->color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
          
          // Hardcoded 35px shift mapping to clear out left static noise
          gfx->writeFillRect((w * blockWidth) + 35, py, blockWidth, blockHeight, color16);
        }
      }
      gfx->endWrite();
    }

    void loop() override {
      if (!initDone || !gfx) return;

      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        lastPowerState = currentPowerState;
        int8_t pinBl = 14;
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
