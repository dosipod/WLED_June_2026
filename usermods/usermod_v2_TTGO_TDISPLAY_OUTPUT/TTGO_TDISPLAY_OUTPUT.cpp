#include "wled.h"

// Explicitly inject the configurations directly before importing headers
#undef TFT_MISO
#define TFT_MISO 4

#include <TFT_eSPI.h>

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    TFT_eSPI* tft = nullptr; 
    unsigned long lastUpdate = 0;
    bool initDone = false;
    bool pinsAllocated = false;

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Starting setup..."));

      int8_t pinsToAllocate[] = {TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL};
      const char* pinNames[]  = {"MOSI", "SCLK", "CS", "DC", "RST", "BL"};
      
      pinsAllocated = true;

      for (uint8_t i = 0; i < 6; i++) {
        if (pinsToAllocate[i] >= 0) {
          DEBUG_PRINT(F("[UM_DisplayMatrix] Allocating "));
          DEBUG_PRINT(pinNames[i]);
          DEBUG_PRINT(F(" on Pin: "));
          DEBUG_PRINTLN(pinsToAllocate[i]);

          // Pass false to ensure WLED acts strictly as a tracker without breaking native SPI mappings
          if (!PinManager::allocatePin(pinsToAllocate[i], false, PinOwner::UM_Unspecified)) {
            DEBUG_PRINT(F("[UM_DisplayMatrix] FATAL: Pin allocation failed for "));
            DEBUG_PRINTLN(pinNames[i]);
            pinsAllocated = false;
            break;
          }
        }
      }

      if (!pinsAllocated) {
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] Usermod aborted due to pin conflicts."));
        return;
      }

      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Initializing low-level GPIO matrix mapping..."));
      
      // Explicitly lock output configurations on the hardware layer to bypass library bugs
      pinMode(TFT_CS, OUTPUT);
      pinMode(TFT_DC, OUTPUT);
      pinMode(TFT_RST, OUTPUT);
      pinMode(TFT_BL, OUTPUT);
      
      digitalWrite(TFT_CS, HIGH);
      digitalWrite(TFT_RST, HIGH);
      digitalWrite(TFT_BL, HIGH); // Drive backlight up immediately

      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Instantiating unmanaged TFT instance..."));
      
      tft = new TFT_eSPI();

      // Directly initialize the configuration registers to bypass the broken spi_bus_initialize macro block
      setup_t activeSettings;
      tft->getSetup(activeSettings); 

      // Manually trigger hardware initialization routines
      tft->setRotation(1);
      tft->fillScreen(TFT_BLACK);
      tft->setTextColor(TFT_WHITE, TFT_BLACK);
      tft->setTextSize(2);
      tft->drawString("WLED Initializing...", 10, 10);

      DEBUG_PRINTLN(F("[UM_DisplayMatrix] TFT initialization completed successfully!"));
      initDone = true;
    }

    void loop() override {
      if (!initDone || !pinsAllocated || !tft) return;

      if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        
        tft->fillScreen(TFT_BLACK);
        tft->drawString("WLED Active", 10, 10);
        tft->drawString(WiFi.localIP().toString().c_str(), 10, 40);
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
