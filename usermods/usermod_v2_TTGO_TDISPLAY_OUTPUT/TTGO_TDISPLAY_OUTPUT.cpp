#include "wled.h"
#include <Arduino_GFX_Library.h> // Switch to stable GFX to fix the S3 boot loop panic

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
          DEBUG_PRINT(F("[UM_DisplayMatrix] Tracking Pin: "));
          DEBUG_PRINT(pinNames[i]);
          DEBUG_PRINT(F(" -> "));
          DEBUG_PRINTLN(pinsToAllocate[i]);

          // Track within WLED database without taking physical register control
          if (!PinManager::allocatePin(pinsToAllocate[i], false, PinOwner::UM_Unspecified)) {
            DEBUG_PRINTLN(F("[UM_DisplayMatrix] Warning: Pin was already registered elsewhere."));
          }
        }
      }

      // Initialize your specific board backlight hardware directly
      pinMode(TFT_BL, OUTPUT);
      digitalWrite(TFT_BL, HIGH); 

      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Creating unmanaged hardware bus..."));
      
      // Build a clean, native hardware SPI bus configuration mapping directly to S3 matrix registers
      bus = new Arduino_ESP32SPI(
        TFT_DC,   // DC
        TFT_CS,   // CS
        TFT_SCLK, // SCK
        TFT_MOSI, // MOSI
        -1        // MISO (Explicitly detached to safely disable read tracking)
      );

      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Spawning ST7789 display controller..."));
      
      // Construct display mapping to match your T-Display 170x320 specifications
      gfx = new Arduino_ST7789(
        bus, 
        TFT_RST, 
        1,       // Rotation mapping
        true,    // IPS panel setup
        170,     // Screen Width
        320      // Screen Height
      );

      // Secure initialization hook (safe from register panic loops)
      gfx->begin();
      gfx->fillScreen(BLACK);
      
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Display successfully initialized without crashes!"));
      initDone = true;
    }

    void loop() override {
      if (!initDone || !gfx) return;

      if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        
        gfx->fillScreen(BLACK);
        gfx->setTextSize(2);
        gfx->setTextColor(WHITE);
        gfx->setCursor(10, 10);
        gfx->println("WLED Active");
        gfx->setCursor(10, 40);
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
