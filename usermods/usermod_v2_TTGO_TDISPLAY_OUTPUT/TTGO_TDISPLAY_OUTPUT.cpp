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
#endif#include "wled.h"

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

#if defined(USER_SETUP_LOADED) || defined(TFT_CS)
  #include "lcd_as_output_espi_engine.h"
  #define IS_ESPI_ACTIVE 1
#else
  #include "lcd_as_output_gfx_engine.h"
  #define IS_ESPI_ACTIVE 0
#endif

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    #if (IS_ESPI_ACTIVE == 1)
      LcdTfteSpiEngine* engine = nullptr;
    #else
      LcdGfxEngine* engine = nullptr;
    #endif

    int selectedProfile = 1; 

    uint16_t displayWidth = 320;
    uint16_t displayHeight = 170;
    int16_t colOffset = 35;
    
    int8_t pinMosi = -1;
    int8_t pinSclk = -1;
    int8_t pinCs   = -1;
    int8_t pinDc   = -1;
    int8_t pinRst  = -1;
    int8_t pinBl   = -1;

    bool initDone = false;
    bool lastPowerState = true;

    void applyHardwareProfile() {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] applyHardwareProfile() called. Setting profile presets..."));
      if (selectedProfile == 1) { 
        displayWidth  = 320;
        displayHeight = 170;
        colOffset     = 35;
        pinMosi       = 13;
        pinSclk       = 12;
        pinCs         = 10;
        pinDc         = 11;
        pinRst        = 1;
        pinBl         = 14;
      }
    }

    void initializeHardware() {
      if (initDone) return;
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] >>> initializeHardware() START >>>"));

      if (pinMosi >= 0) {
        int8_t pinsToAllocate[] = { pinMosi, pinSclk, pinCs, pinDc, pinRst, pinBl };
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] Attempting PinManager tracking allocations..."));
        for (uint8_t i = 0; i < 6; i++) {
          if (pinsToAllocate[i] >= 0) {
            bool success = PinManager::allocatePin(pinsToAllocate[i], false, PinOwner::UM_Unspecified);
            DEBUG_PRINTF("[UM_DisplayMatrix] Pin Manager Allocate GPIO %d -> Status: %s\n", pinsToAllocate[i], success ? "SUCCESS" : "FAILED/BUSY");
          }
        }
      }

      if (pinBl >= 0) {
        DEBUG_PRINTF("[UM_DisplayMatrix] Initializing Backlight Pin GPIO %d\n", pinBl);
        pinMode(pinBl, OUTPUT);
        digitalWrite(pinBl, HIGH);
      }

      #if (IS_ESPI_ACTIVE == 1)
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] Instantiating Engine Target: TFT_eSPI (Heap Pointer)"));
        engine = new LcdTfteSpiEngine();
        if (engine) {
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] Invoking engine->init() for TFT_eSPI..."));
          engine->init();
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] engine->init() TFT_eSPI Completed successfully."));
        } else {
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] CRITICAL ERROR: Out of Memory constructing TFT_eSPI class layout instance!"));
        }
      #else
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] Instantiating Engine Target: Arduino_GFX (Heap Pointer)"));
        engine = new LcdGfxEngine();
        if (engine) {
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] Invoking engine->init() for Arduino_GFX..."));
          engine->init(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] engine->init() Arduino_GFX Completed successfully."));
        } else {
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] CRITICAL ERROR: Out of Memory constructing Arduino_GFX class layout instance!"));
        }
      #endif

      initDone = true;
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] <<< initializeHardware() END <<<"));
    }

  public:
    void setup() override {
      // VERSION LOG EMBED BLOCK:
      DEBUG_PRINTLN(F("===================================================================="));
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] COMPILE VERSION FOOTPRINT: v2026.07.26-ALPHA-01"));
      DEBUG_PRINTLN(F("===================================================================="));
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] setup() hook triggered."));

      #ifdef TFT_WIDTH
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] Compile-time platformio.ini macro overrides detected. Overwriting local defaults."));
        displayWidth = (uint16_t)TFT_WIDTH;
        displayHeight = (uint16_t)TFT_HEIGHT;
        pinMosi = (int8_t)TFT_MOSI;
        pinSclk = (int8_t)TFT_SCLK;
        pinCs   = (int8_t)TFT_CS;
        pinDc   = (int8_t)TFT_DC;
        pinRst  = (int8_t)TFT_RST;
        pinBl   = (int8_t)TFT_BL;
        selectedProfile = 0;
      #else
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] No platformio.ini macros active. Dropping through to profile detection presets..."));
        applyHardwareProfile();
      #endif

      DEBUG_PRINTF("[UM_DisplayMatrix] Passive setup parsing finished. Dimensions: %dx%d. Pins: MOSI:%d SCLK:%d CS:%d DC:%d RST:%d BL:%d\n", 
                    displayWidth, displayHeight, pinMosi, pinSclk, pinCs, pinDc, pinRst, pinBl);
    }

    void handleOverlayDraw() override {
      // Verifies if the frame is trying to draw before initialization finishes or if a null engine pointer exists
      if (!initDone || !engine) return;
      
      if (!lastPowerState) return;
      
      // Trace entries into frame buffer rendering arrays
      engine->drawFrame(strip);
    }

    void loop() override {
      // Lazy background caller tracking check points
      if (!initDone) {
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] Loop thread discovered uninitialized hardware state. Handing over to initializeHardware()..."));
        initializeHardware();
      }

      if (!initDone) return;

      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        DEBUG_PRINTF("[UM_DisplayMatrix] Display state toggled from brightness index shift. Power state changed to: %s\n", currentPowerState ? "ON" : "OFF");
        lastPowerState = currentPowerState;
        if (pinBl >= 0) digitalWrite(pinBl, lastPowerState ? HIGH : LOW);
        if (!lastPowerState && engine) {
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] Power state drop request. Invoking engine->clear()..."));
          engine->clear();
        }
      }
    }

    void addToConfig(JsonObject& root) override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] addToConfig() JSON writing triggered."));
      JsonObject top = root.createNestedObject(F("DisplayMatrix"));
      top[F("Hardware-Profile")] = selectedProfile; 
      top[F("Display-Width")]    = displayWidth;
      top[F("Display-Height")]   = displayHeight;
      top[F("Column-Offset")]    = colOffset;
      top[F("Pin-MOSI")]         = pinMosi;
      top[F("Pin-SCLK")]         = pinSclk;
      top[F("Pin-CS")]           = pinCs;
      top[F("Pin-DC")]           = pinDc;
      top[F("Pin-RST")]          = pinRst;
      top[F("Pin-Backlight")]    = pinBl;
    }

    bool readFromConfig(JsonObject& root) override {
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] readFromConfig() JSON parsing sequence triggered."));
      JsonObject top = root[F("DisplayMatrix")];
      if (top.isNull()) {
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] readFromConfig FAILED: No config JSON key instance mapping found. Using defaults."));
        return false;
      }

      int oldProfile = selectedProfile;
      if (top[F("Hardware-Profile")].is<int>()) {
        selectedProfile = top[F("Hardware-Profile")].as<int>();
      }

      if (selectedProfile != oldProfile && selectedProfile > 0) {
        DEBUG_PRINTF("[UM_DisplayMatrix] Profile change detected inside config layout (%d -> %d). Resetting driver states.\n", oldProfile, selectedProfile);
        applyHardwareProfile();
        if (engine) {
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] Tearing down active engine structures to reconfigure hardware..."));
          delete engine;
          engine = nullptr;
        }
        initDone = false; 
      } else {
        DEBUG_PRINTLN(F("[UM_DisplayMatrix] Parsing updated settings data nodes..."));
        if (top[F("Display-Width")].is<int>())    displayWidth  = (uint16_t)top[F("Display-Width")].as<int>();
        if (top[F("Display-Height")].is<int>())   displayHeight = (uint16_t)top[F("Display-Height")].as<int>();
        if (top[F("Column-Offset")].is<int>())    colOffset     = (int16_t)top[F("Column-Offset")].as<int>();
        if (top[F("Pin-MOSI")].is<int>())         pinMosi       = (int8_t)top[F("Pin-MOSI")].as<int>();
        if (top[F("Pin-SCLK")].is<int>())         pinSclk       = (int8_t)top[F("Pin-SCLK")].as<int>();
        if (top[F("Pin-CS")].is<int>())           pinCs         = (int8_t)top[F("Pin-CS")].as<int>();
        if (top[F("Pin-DC")].is<int>())           pinDc         = (int8_t)top[F("Pin-DC")].as<int>();
        if (top[F("Pin-RST")].is<int>())          pinRst        = (int8_t)top[F("Pin-RST")].as<int>();
        if (top[F("Pin-Backlight")].is<int>())    pinBl         = (int8_t)top[F("Pin-Backlight")].as<int>();

        if (initDone && engine) {
          DEBUG_PRINTLN(F("[UM_DisplayMatrix] Live settings applied while active. Re-triggering driver init sequence..."));
          #if (IS_ESPI_ACTIVE == 1)
            engine->init();
          #else
            engine->init(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
          #endif
        }
      }
      DEBUG_PRINTLN(F("[UM_DisplayMatrix] readFromConfig() parsing complete."));
      return true;
    }

    void addToJsonState(JsonObject& root) override {
      JsonObject top = root.createNestedObject("TTGO_Display");
      top["active"] = initDone;
    }

    uint16_t getId() override {
      #ifdef USERMOD_ID_TTGO_TDISPLAY_OUTPUT
        return USERMOD_ID_TTGO_TDISPLAY_OUTPUT;
      #else
        return USERMOD_ID_UNSPECIFIED;
      #endif
    }

    ~TTGO_TDISPLAY_OUTPUT() {
      if (engine) {
        delete engine;
        engine = nullptr;
      }
    }
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);

#ifdef WHITE
  #undef WHITE
#endif

#if defined(USER_SETUP_LOADED) || defined(TFT_CS)
  #include "lcd_as_output_espi_engine.h"
  #define IS_ESPI_ACTIVE 1
#else
  #include "lcd_as_output_gfx_engine.h"
  #define IS_ESPI_ACTIVE 0
#endif

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    // STRICT POINTER TYPE DECLARATIONS: Keeps class footprint clean at boot to avoid early bus lockups
    #if (IS_ESPI_ACTIVE == 1)
      LcdTfteSpiEngine* engine = nullptr;
    #else
      LcdGfxEngine* engine = nullptr;
    #endif

    int selectedProfile = 1; 

    uint16_t displayWidth = 320;
    uint16_t displayHeight = 170;
    int16_t colOffset = 35;
    
    int8_t pinMosi = -1;
    int8_t pinSclk = -1;
    int8_t pinCs   = -1;
    int8_t pinDc   = -1;
    int8_t pinRst  = -1;
    int8_t pinBl   = -1;

    bool initDone = false;
    bool lastPowerState = true;

    void applyHardwareProfile() {
      if (selectedProfile == 1) { 
        displayWidth  = 320;
        displayHeight = 170;
        colOffset     = 35;
        pinMosi       = 13;
        pinSclk       = 12;
        pinCs         = 10;
        pinDc         = 11;
        pinRst        = 1;
        pinBl         = 14;
      }
    }

  public:
    void setup() override {
      #ifdef TFT_WIDTH
        displayWidth = (uint16_t)TFT_WIDTH;
        displayHeight = (uint16_t)TFT_HEIGHT;
        pinMosi = (int8_t)TFT_MOSI;
        pinSclk = (int8_t)TFT_SCLK;
        pinCs   = (int8_t)TFT_CS;
        pinDc   = (int8_t)TFT_DC;
        pinRst  = (int8_t)TFT_RST;
        pinBl   = (int8_t)TFT_BL;
        selectedProfile = 0;
      #else
        if (selectedProfile == 1 && pinMosi == -1) {
          applyHardwareProfile();
        }
      #endif

      // Safely register pins with WLED database
      if (pinMosi >= 0) {
        int8_t pinsToAllocate[] = { pinMosi, pinSclk, pinCs, pinDc, pinRst, pinBl };
        for (uint8_t i = 0; i < 6; i++) {
          if (pinsToAllocate[i] >= 0) {
            PinManager::allocatePin(pinsToAllocate[i], false, PinOwner::UM_Unspecified);
          }
        }
      }

      if (pinBl >= 0) {
        pinMode(pinBl, OUTPUT);
        digitalWrite(pinBl, HIGH);
      }

      // SAFE RUNTIME ALLOCATION: Instantiate display engine pointer cleanly after strip footprints register
      #if (IS_ESPI_ACTIVE == 1)
        engine = new LcdTfteSpiEngine();
        if (engine) engine->init();
      #else
        engine = new LcdGfxEngine();
        if (engine) engine->init(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
      #endif

      initDone = true;
    }

    void handleOverlayDraw() override {
      // Shield the canvas rendering logic completely unless driver registration is initialized
      if (!initDone || !lastPowerState || !engine) return;
      engine->drawFrame(strip);
    }

    void loop() override {
      if (!initDone) return;

      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        lastPowerState = currentPowerState;
        if (pinBl >= 0) digitalWrite(pinBl, lastPowerState ? HIGH : LOW);
        if (!lastPowerState && engine) {
          engine->clear();
        }
      }
    }

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(F("DisplayMatrix"));
      top[F("Hardware-Profile")] = selectedProfile; 
      top[F("Display-Width")]    = displayWidth;
      top[F("Display-Height")]   = displayHeight;
      top[F("Column-Offset")]    = colOffset;
      top[F("Pin-MOSI")]         = pinMosi;
      top[F("Pin-SCLK")]         = pinSclk;
      top[F("Pin-CS")]           = pinCs;
      top[F("Pin-DC")]           = pinDc;
      top[F("Pin-RST")]          = pinRst;
      top[F("Pin-Backlight")]    = pinBl;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[F("DisplayMatrix")];
      if (top.isNull()) return false;

      int oldProfile = selectedProfile;
      if (top[F("Hardware-Profile")].is<int>()) {
        selectedProfile = top[F("Hardware-Profile")].as<int>();
      }

      if (selectedProfile != oldProfile && selectedProfile > 0) {
        applyHardwareProfile();
        if (engine) {
          #if (IS_ESPI_ACTIVE == 1)
            engine->init();
          #else
            engine->init(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
          #endif
        }
      } else {
        if (top[F("Display-Width")].is<int>())    displayWidth  = (uint16_t)top[F("Display-Width")].as<int>();
        if (top[F("Display-Height")].is<int>())   displayHeight = (uint16_t)top[F("Display-Height")].as<int>();
        if (top[F("Column-Offset")].is<int>())    colOffset     = (int16_t)top[F("Column-Offset")].as<int>();
        if (top[F("Pin-MOSI")].is<int>())         pinMosi       = (int8_t)top[F("Pin-MOSI")].as<int>();
        if (top[F("Pin-SCLK")].is<int>())         pinSclk       = (int8_t)top[F("Pin-SCLK")].as<int>();
        if (top[F("Pin-CS")].is<int>())           pinCs         = (int8_t)top[F("Pin-CS")].as<int>();
        if (top[F("Pin-DC")].is<int>())           pinDc         = (int8_t)top[F("Pin-DC")].as<int>();
        if (top[F("Pin-RST")].is<int>())          pinRst        = (int8_t)top[F("Pin-RST")].as<int>();
        if (top[F("Pin-Backlight")].is<int>())    pinBl         = (int8_t)top[F("Pin-Backlight")].as<int>();

        if (initDone && engine) {
          #if (IS_ESPI_ACTIVE == 1)
            engine->init();
          #else
            engine->init(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
          #endif
        }
      }
      return true;
    }

    void addToJsonState(JsonObject& root) override {
      JsonObject top = root.createNestedObject("TTGO_Display");
      top["active"] = initDone;
    }

    uint16_t getId() override {
      #ifdef USERMOD_ID_TTGO_TDISPLAY_OUTPUT
        return USERMOD_ID_TTGO_TDISPLAY_OUTPUT;
      #else
        return USERMOD_ID_UNSPECIFIED;
      #endif
    }

    ~TTGO_TDISPLAY_OUTPUT() {
      if (engine) {
        delete engine;
        engine = nullptr;
      }
    }
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);
