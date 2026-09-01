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


#ifdef ST7789_SIZE_320X170
  // S3 Setup Parameters
  const uint16_t targetWidth  = 320;
  const uint16_t targetHeight = 170;
#elif ST7789_SIZE_240X134
  // Fallback / Classic TTGO T-Display Setup Parameters
  const uint16_t targetWidth  = 240;
  const uint16_t targetHeight = 135;
#elif ILI9341_2_SIZE_320X240
  // CYD Setup Parameters
  const uint16_t targetWidth  = 320;
  const uint16_t targetHeight = 240;
#elif ST7735_SIZE_160x128
  // SIZE_160x128
  const uint16_t targetWidth  = 160;
  const uint16_t targetHeight = 128;
#elif ST7735_SIZE_128x128
  const uint16_t targetWidth  = 128;
  const uint16_t targetHeight = 128;
#else
 /// TODO ADD MORE DISPLAY
#endif

    
    
    uint16_t blockWidth = 1;
    uint16_t blockHeight = 1;
    
    bool initDone = false;
    bool lastPowerState = true;

    void initializeDisplay() {
      if (initDone) return;

      // Extract your native environment pin definitions from the ini file 
    #ifdef ST7735_SIZE_160x128
    // Use the new, unique macros defined in platformio.ini
      int8_t pinMosi = (int8_t) MY_PIN_MOSI;
      int8_t pinSclk = (int8_t) MY_PIN_SCLK;
      int8_t pinCs   = (int8_t) MY_PIN_CS;
      int8_t pinDc   = (int8_t) MY_PIN_DC;
      int8_t pinRst  = (int8_t) MY_PIN_RST;
      int8_t pinBl   = (int8_t) MY_PIN_BL;
    #else // Fallback constants for other functional environments (S3 / CYD)
      int8_t pinMosi = (int8_t)TFT_MOSI;
      int8_t pinSclk = (int8_t)TFT_SCLK;
      int8_t pinCs   = (int8_t)TFT_CS;
      int8_t pinDc   = (int8_t)TFT_DC;
      int8_t pinRst  = (int8_t)TFT_RST;
      int8_t pinBl   = (int8_t)TFT_BL;
   #endif
      
      PinManager::allocatePin(pinMosi, false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinSclk, false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinCs,   false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinDc,   false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinRst,  false, PinOwner::UM_Unspecified);
      PinManager::allocatePin(pinBl,   false, PinOwner::UM_Unspecified);

      pinMode(pinBl, OUTPUT);
      digitalWrite(pinBl, HIGH);

      #if defined(CONFIG_IDF_TARGET_ESP32C3)
      bus = new Arduino_SWSPI(pinDc, pinCs, pinSclk, pinMosi, -1);
      #else
      bus = new Arduino_ESP32SPI(pinDc, pinCs, pinSclk, pinMosi, -1);
      #endif


      // AUTOMATED OFFSET MAPPING BOUNDS:
      // Evaluates your active -D CGRAM_OFFSET flag from your ini file. If active, it 
      // automatically shifts display tracking by 35px to clear screen alignment errors.
      #ifdef CGRAM_OFFSET
        int16_t currentOffset = 35;
      #else
        int16_t currentOffset = 0;
      #endif

      // Strict INI-Driven Driver Allocation footprint map

      #ifdef ST7789_SIZE_320X170
      
      
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


    #elif ST7789_SIZE_240X134 
    // The older 240x135 display maps explicitly shifted coordinates
    int16_t col_offset1 = 52; 
    int16_t row_offset1 = 40;
    int16_t col_offset2 = 53;
    int16_t row_offset2 = 40;

  gfx = new Arduino_ST7789(
    bus, 
    pinRst, 
    1,            // Rotation set to Landscape
    true,         // IPS Panel flag
    135,          // Physical matrix panel structural width
    240,          // Physical matrix panel structural height
    col_offset1, 
    row_offset1, 
    col_offset2, 
    row_offset2
    );

  #elif ILI9341_2_SIZE_320X240 

  // Correct structural call matching the Arduino_GFX repository
  gfx = new Arduino_ILI9341(
    bus, 
    pinRst, 
    1,            // Rotation set to 1 (Landscape)
    false         // Standard TN panel flag (CYD is not IPS)
  );

#elif ST7735_SIZE_160x128
  // WAVGAT 1.8 utilizes an ST7735 class constructor tracking native parameters
  gfx = new Arduino_ST7735(
    bus, 
    pinRst, 
    1,            // Rotation set to 1 (Landscape)
    false,        // Standard panel target (Not IPS layout)
    128,          // Physical Canvas width configuration
    160,          // Physical Canvas height configuration
    0,            // Column hardware shift offset
    0,            // Row hardware shift offset
    0,            // Alternative column hardware shift offset
    0,            // Alternative row hardware shift offset
    false         // BGR color layout flag toggling configuration
  );

#elif ST7735_SIZE_128x128
  // TDO
  gfx = new Arduino_ST7735(
    bus, 
    pinRst, 
    3,            // Rotation set to 1 (Landscape)
    false,        // Standard panel target (Not IPS layout)
    128,          // Physical Canvas width configuration
    128,          // Physical Canvas height configuration
    3,       // Column hardware shift offset 
    1,       // Row hardware shift offset   
    0,            // Alternative column hardware shift offset
    0,            // Alternative row hardware shift offset
    true         // BGR color layout flag toggling configuration
  );




      
  #else
    // new display specs here
  #endif





      
      if (gfx) {
        gfx->begin();
        gfx->setRotation(1); // Force landscape transformation loop matching targetWidth/targetHeight
        gfx->fillScreen(RGB565_BLACK);
        initDone = true;

         #ifdef ILI9341_2_SIZE_320X240
         // Toggles hardware-level coordinate color decoding registers to fix inverted RGB layers
         gfx->invertDisplay(true); 
        #endif
        
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
