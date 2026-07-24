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

      // INITIALIZE IN NATIVE PORTRAIT MODE:
      // Setting Rotation to 0 maps the hardware frame window cleanly over the 170x320 controller RAM,
      // which eliminates the automatic landscape offset shifts that are causing the static bars.
      gfx = new Arduino_ST7789(
        bus, 
        TFT_RST, 
        0,     // Native Portrait Rotation to lock register tables
        true,  // Active IPS Color Mapping Flag
        170,   // Native Panel Width
        320,   // Native Panel Height
        0,     // 0 Column Offset
        0      // 0 Row Offset
      );

      gfx->begin();
      gfx->fillScreen(RGB565_BLACK); 
      
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Setup Complete. Canvas cleared."));
      initDone = true;
    }

    void handleOverlayDraw() override {
      if (!initDone || !gfx || !lastPowerState) return;

      uint16_t matrixWidth  = strip.isMatrix ? Segment::maxWidth  : 1;
      uint16_t matrixHeight = strip.isMatrix ? Segment::maxHeight : 1;

      if (matrixWidth == 0 || matrixHeight == 0) return;

      Segment& seg = strip.getSegment(0);

      gfx->startWrite();
      
      // Since the driver runs at 170x320 portrait, we map WLED's landscape 
      // width to the driver's height (320), and WLED's landscape height to the driver's width (170).
      float displayWidth  = 170.0f;
      float displayHeight = 320.0f;

      float scaleX = displayWidth / (float)matrixHeight; // Inverted mapping step
      float scaleY = displayHeight / (float)matrixWidth;  // Inverted mapping step
      
      for (uint16_t y = 0; y < matrixHeight; y++) {
        for (uint16_t x = 0; x < matrixWidth; x++) {
          
          uint32_t c = seg.getPixelColorXY(x, y);
          
          uint8_t r = (c >> 16) & 0xFF;
          uint8_t g = (c >> 8)  & 0xFF;
          uint8_t b = c         & 0xFF;
          
          uint16_t color16 = gfx->color565(r, g, b);
          
          // MANUALLY ROTATE ROTATION 1 COORDINATES (Landscape):
          // Map incoming landscape matrix values (X, Y) to portrait screen registers (new_x, new_y)
          // formula: new_x = y, new_y = (matrixWidth - 1 - x)
          uint16_t rotX = y;
          uint16_t rotY = matrixWidth - 1 - x;

          uint16_t px = (uint16_t)(rotX * scaleX);
          uint16_t py = (uint16_t)(rotY * scaleY);
          uint16_t pw = (uint16_t)((rotX + 1) * scaleX) - px;
          uint16_t ph = (uint16_t)((rotY + 1) * scaleY) - py;

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

      if (!lastPowerState) return;

      if (millis() - lastUpdate > 5000) {
        lastUpdate = millis();
        gfx->setTextSize(2);
        gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
        // Adjusted text cursor to match the manual orientation layout
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
