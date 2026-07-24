#include "wled.h"
#include <TFT_eSPI.h>

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    TFT_eSPI tft = TFT_eSPI();
    unsigned long lastUpdate = 0;
    bool initDone = false;
    bool pinsAllocated = false;

  public:
    void setup() override {
      USER_PRINTLN(F("[UM_DisplayMatrix] Starting setup..."));

      // Array of configuration pins from platformio.ini
      int8_t pinsToAllocate[] = {TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL};
      const char* pinNames[]  = {"MOSI", "SCLK", "CS", "DC", "RST", "BL"};
      
      pinsAllocated = true;

      // Safe registration loop using native WLED pinManager mechanics
      for (uint8_t i = 0; i < 6; i++) {
        if (pinsToAllocate[i] >= 0) {
          USER_PRINT(F("[UM_DisplayMatrix] Allocating "));
          USER_PRINT(pinNames[i]);
          USER_PRINT(F(" on Pin: "));
          USER_PRINTLN(pinsToAllocate[i]);

          // Use UM_Unspecified to ensure seamless compatibility without core changes
          if (!pinManager.allocatePin(pinsToAllocate[i], true, PinOwner::UM_Unspecified)) {
            USER_PRINT(F("[UM_DisplayMatrix] FATAL: Pin allocation failed for "));
            USER_PRINTLN(pinNames[i]);
            pinsAllocated = false;
            break;
          }
        }
      }

      if (!pinsAllocated) {
        USER_PRINTLN(F("[UM_DisplayMatrix] Usermod aborted due to pin conflicts."));
        return;
      }

      USER_PRINTLN(F("[UM_DisplayMatrix] All pins allocated successfully. Initializing TFT..."));

      // Initialize hardware display
      tft.init();
      tft.setRotation(1);
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);
      tft.drawString("WLED Initializing...", 10, 10);

      USER_PRINTLN(F("[UM_DisplayMatrix] TFT initialized successfully!"));
      initDone = true;
    }

    void loop() override {
      if (!initDone || !pinsAllocated) return;

      // Throttle display updates to every 1000ms to keep main loop fast
      if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        
        // Simple screen update to confirm it runs alongside Wi-Fi
        tft.fillScreen(TFT_BLACK);
        tft.drawString("WLED Active", 10, 10);
        tft.drawString(WiFi.localIP().toString().c_str(), 10, 40);
      }
    }

    void addToJsonState(JsonObject& root) override {
      JsonObject top = root.createNestedObject("TTGO_Display");
      top["active"] = initDone;
    }

    uint16_t getId() override {
      return USERMOD_ID_TTGO_TDISPLAY;
    }
};

// Official registration format compliant with the modern WLED specification
static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);
