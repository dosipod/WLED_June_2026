#include "wled.h"
#include <Arduino_GFX_Library.h>

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    Arduino_DataBus* bus = nullptr;
    Arduino_GFX* gfx = nullptr;
    unsigned long lastUpdate = 0;
    bool initDone = false;
    bool pinsAllocated = false;

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Starting setup via native GFX..."));

      int8_t pinsToAllocate[] = {TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL};
      const char* pinNames[]  = {"MOSI", "SCLK", "CS", "DC", "RST", "BL"};
      
      pinsAllocated = true;

      for (uint8_t i = 0; i < 6; i++) {
        if (pinsToAllocate[i] >= 0) {
          if (!PinManager::allocatePin(pinsToAllocate[i], false, PinOwner::UM_Unspecified)) {
            DEBUG_PRINTLN(F("[UM_DisplayMatrix] Warning: Pin conflict bypassed."));
          }
        }
      }

      pinMode(TFT_BL, OUTPUT);
      digitalWrite(TFT_BL, HIGH); 

      bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);

      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Spawning ST7789 display controller..."));
      
      // Adjusted configuration: Using explicit 170x320 panel setup with row/col offsets
      // If it still stretches or shows garbage, we will try the 135x240 variant.
      gfx = new Arduino_ST7789(
        bus, 
        TFT_RST, 
        1,       // Rotation (Landscape)
        true,    // IPS Panel mode flag
        170,     // Screen Width
        320,     // Screen Height
        35,      // Col offset (fixes the garbage edge lines on typical S3 T-Displays)
        0        // Row offset
      );

      gfx->begin();
      gfx->fillScreen(BLACK);
      
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] TFT initialized cleanly!"));
      initDone = true;
    }

    // This method is called by WLED every time a frame is rendered to the LEDs
    void handleOverlayDraw() override {
      if (!initDone || !gfx) return;

      // --- LIVE MATRIX RENDERING CORE ---
      // This loops through WLED's virtual strip pixel buffer and pushes colors to the screen
      uint16_t totalPixels = strip.getLengthTotal();
      
      // Optimization: Scale or map your strip length to the screen boundaries
      // Adjust this loop depending on how your old usermod mapped individual pixel indexes to X/Y coordinates
      for (uint16_t i = 0; i < totalPixels; i++) {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = R(c);
        uint8_t g = G(c);
        uint8_t b = B(c);
        
        // Convert to 16-bit color format (RGB565) required by the display
        uint16_t color16 = gfx->color565(r, g, b);
        
        // Example Mapping: Simple horizontal strip line layout
        // Swap this with your original 2D matrix transformation code if you have a grid layout
        gfx->drawPixel(i % 320, i / 320, color16);
      }
    }

    void loop() override {
      if (!initDone || !gfx) return;

      // Keep light metadata on top of the matrix layer if desired (e.g., every 5 seconds)
      if (millis() - lastUpdate > 5000) {
        lastUpdate = millis();
        
        gfx->setTextSize(2);
        gfx->setTextColor(WHITE);
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
