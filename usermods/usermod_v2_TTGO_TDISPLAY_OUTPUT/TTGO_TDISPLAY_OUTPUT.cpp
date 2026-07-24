#include "wled.h"

// Isolate and clear WLED 32-bit color macro definitions to prevent library naming conflicts
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

      // Configure tracking variables for all display control pins
      int8_t pinsToAllocate[] = {TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL};
      pinsAllocated = true;

      for (uint8_t i = 0; i < 6; i++) {
        if (pinsToAllocate[i] >= 0) {
          // Track pins in the WLED framework as an unmanaged resource to avoid conflicts
          PinManager::allocatePin(pinsToAllocate[i], false, PinOwner::UM_Unspecified);
        }
      }

      // Initialize the display backlight pin and drive it high immediately
      pinMode(TFT_BL, OUTPUT);
      digitalWrite(TFT_BL, HIGH); 

      // Instantiate the low-level hardware SPI bus connection layout
      bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);

      // TARGETED CANVAS LAYOUT CONFIGURATION:
      // Setting width to 135 and height to 280 drops the unmapped memory region.
      // An offset of 52 shifts calculations cleanly past the uninitialized RAM boundary.
      gfx = new Arduino_ST7789(
        bus, 
        TFT_RST, 
        1,     // Rotation: 1 (Landscape Orientation)
        true,  // Active IPS Color Mapping Flag
        135,   // Physical internal frame width
        280,   // Physical internal active frame height
        52,    // 52 Column hardware matrix offset assignment
        0      // 0 Row offset
      );

      gfx->begin();
      gfx->fillScreen(RGB565_BLACK); // Flush frame buffer data safely to black
      
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Setup Complete. Canvas cleared."));
      initDone = true;
    }

    void handleOverlayDraw() override {
      if (!initDone || !gfx || !lastPowerState) return;

      // Extract active virtual matrix constraints natively from WLED layout configurations
      uint16_t matrixWidth  = strip.isMatrix ? Segment::maxWidth  : 1;
      uint16_t matrixHeight = strip.isMatrix ? Segment::maxHeight : 1;

      if (matrixWidth == 0 || matrixHeight == 0) return;

      // Extract the primary segment parameters to handle clean 2D layout calculation blocks
      Segment& seg = strip.getSegment(0);

      gfx->startWrite();
      
      // Calculate scaling multipliers dynamically relative to the rotated driver coordinate metrics
      float displayWidth  = (float)gfx->width();
      float displayHeight = (float)gfx->height();

      float scaleX = displayWidth / (float)matrixWidth;
      float scaleY = displayHeight / (float)matrixHeight;
      
      // Multi-axis coordinate mapping loops processing the frame modifications sequentially
      for (uint16_t y = 0; y < matrixHeight; y++) {
        for (uint16_t x = 0; x < matrixWidth; x++) {
          
          // Pull 2D coordinate colors respecting custom routing structures (like serpentine paths)
          uint32_t c = seg.getPixelColorXY(x, y);
          
          // Deconstruct 32-bit RGB data blocks safely
          uint8_t r = (c >> 16) & 0xFF;
          uint8_t g = (c >> 8)  & 0xFF;
          uint8_t b = c         & 0xFF;
          
          // Translate parameters into standard 16-bit color format (RGB565)
          uint16_t color16 = gfx->color565(r, g, b);
          
          // Project individual matrix points cleanly into scaled blocks that fill the screen boundary
          uint16_t px = (uint16_t)(x * scaleX);
          uint16_t py = (uint16_t)(y * scaleY);
          uint16_t pw = (uint16_t)((x + 1) * scaleX) - px;
          uint16_t ph = (uint16_t)((y + 1) * scaleY) - py;

          // Draw the calculated color block straight onto the screen canvas layout
          gfx->writeFillRect(px, py, pw, ph, color16);
        }
      }
      
      gfx->endWrite();
    }

    void loop() override {
      if (!initDone || !gfx) return;

      // Synchronize display power directly with WLED's main UI brightness setting
      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        lastPowerState = currentPowerState;
        digitalWrite(TFT_BL, lastPowerState ? HIGH : LOW);
        if (!lastPowerState) {
          gfx->fillScreen(RGB565_BLACK);
        }
      }

      if (!lastPowerState) return;

      // Superimpose the current device IP address onto the display area every 5 seconds
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
