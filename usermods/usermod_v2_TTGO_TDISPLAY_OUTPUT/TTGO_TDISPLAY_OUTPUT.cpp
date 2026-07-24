#include "wled.h"

// Macro namespace cleanup to safeguard compiling cross-dependencies
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

// CLEAN ARCHITECTURE GUARD BLOCKS:
// Dynamically checks environment build flags before attempting header loads.
// This allows other environments to build smoothly without hardcoding header errors.
#ifdef USER_SETUP_LOADED
  #include "lcd_as_output_espi_engine.h"
#else
  #include "lcd_as_output_gfx_engine.h"
#endif

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    #ifdef USER_SETUP_LOADED
      LcdTfteSpiEngine engine;
      const int activeDriverMode = 0; // Fixed to TFT_eSPI mode natively
    #else
      LcdGfxEngine engine;
      const int activeDriverMode = 1; // Fixed to Arduino_GFX mode natively
    #endif
    
    bool initDone = false;
    bool pinsAllocated = false;
    bool lastPowerState = true;

  public:
    void setup() override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] Launching environment-aware output usermod..."));

      int8_t pinsToAllocate[] = {TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL};
      pinsAllocated = true;
      for (uint8_t i = 0; i < 6; i++) {
        if (pinsToAllocate[i] >= 0) {
          PinManager::allocatePin(pinsToAllocate[i], false, PinOwner::UM_Unspecified);
        }
      }

      pinMode(TFT_BL, OUTPUT);
      digitalWrite(TFT_BL, HIGH);

      // Initialize the precise engine instance active in your current build target
      #ifdef USER_SETUP_LOADED
        engine.init();
      #else
        engine.init(TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_WIDTH, TFT_HEIGHT);
      #endif

      initDone = true;
    }

    void handleOverlayDraw() override {
      if (!initDone || !lastPowerState) return;
      engine.drawFrame(strip);
    }

    void loop() override {
      if (!initDone) return;
      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        lastPowerState = currentPowerState;
        digitalWrite(TFT_BL, lastPowerState ? HIGH : LOW);
        if (!lastPowerState) {
          engine.clear();
        }
      }
    }

    void addToJsonState(JsonObject& root) override {
      JsonObject top = root.createNestedObject("TTGO_Display");
      top["active"] = initDone;
      top["engine_mode"] = activeDriverMode;
    }

    uint16_t getId() override {
      return USERMOD_ID_TTGO_TDISPLAY_OUTPUT;
    }
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);
