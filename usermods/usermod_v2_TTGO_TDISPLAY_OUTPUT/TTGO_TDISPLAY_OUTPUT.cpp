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

#if defined(USER_SETUP_LOADED) || defined(TFT_CS)
  #include "lcd_as_output_espi_engine.h"
  #define IS_ESPI_ACTIVE 1
#else
  #include "lcd_as_output_gfx_engine.h"
  #define IS_ESPI_ACTIVE 0
#endif

// FIXED USERMOD PARAMETER KEYS DEFINITIONS
#define SETTING_PROFILE "Hardware-Profile"
#define SETTING_WIDTH   "Display-Width"
#define SETTING_HEIGHT  "Display-Height"
#define SETTING_OFFSET  "Column-Offset"
#define PIN_MOSI_KEY    "Pin-MOSI"
#define PIN_SCLK_KEY    "Pin-SCLK"
#define PIN_CS_KEY      "Pin-CS"
#define PIN_DC_KEY      "Pin-DC"
#define PIN_RST_KEY     "Pin-RST"
#define PIN_BL_KEY      "Pin-Backlight"

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    #if (IS_ESPI_ACTIVE == 1)
      LcdTfteSpiEngine engine;
      const int activeDriverMode = 0;
    #else
      LcdGfxEngine engine;
      const int activeDriverMode = 1;
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
        applyHardwareProfile();
      #endif
    }

    void init() override {
      if (initDone) return;

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

      #if (IS_ESPI_ACTIVE == 1)
        engine.init();
      #else
        engine.init(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
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
        if (pinBl >= 0) digitalWrite(pinBl, lastPowerState ? HIGH : LOW);
        if (!lastPowerState) {
          engine.clear();
        }
      }
    }

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(F("DisplayMatrix"));
      top[F(SETTING_PROFILE)] = selectedProfile; 
      top[F(SETTING_WIDTH)]  = displayWidth;
      top[F(SETTING_HEIGHT)] = displayHeight;
      top[F(SETTING_OFFSET)] = colOffset;
      top[F(PIN_MOSI_KEY)]   = pinMosi;
      top[F(PIN_SCLK_KEY)]   = pinSclk;
      top[F(PIN_CS_KEY)]     = pinCs;
      top[F(PIN_DC_KEY)]     = pinDc;
      top[F(PIN_RST_KEY)]    = pinRst;
      top[F(PIN_BL_KEY)]     = pinBl;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[F("DisplayMatrix")];
      if (top.isNull()) return false;

      int oldProfile = selectedProfile;
      if (top[F(SETTING_PROFILE)].is<int>()) {
        selectedProfile = top[F(SETTING_PROFILE)].as<int>();
      }

      if (selectedProfile != oldProfile && selectedProfile > 0) {
        applyHardwareProfile();
      } else {
        if (top[F(SETTING_WIDTH)].is<int>())   displayWidth  = (uint16_t)top[F(SETTING_WIDTH)].as<int>();
        if (top[F(SETTING_HEIGHT)].is<int>())  displayHeight = (uint16_t)top[F(SETTING_HEIGHT)].as<int>();
        if (top[F(SETTING_OFFSET)].is<int>())  colOffset     = (int16_t)top[F(SETTING_OFFSET)].as<int>();
        if (top[F(PIN_MOSI_KEY)].is<int>())    pinMosi       = (int8_t)top[F(PIN_MOSI_KEY)].as<int>();
        if (top[F(PIN_SCLK_KEY)].is<int>())    pinSclk       = (int8_t)top[F(PIN_SCLK_KEY)].as<int>();
        if (top[F(PIN_CS_KEY)].is<int>())      pinCs         = (int8_t)top[F(PIN_CS_KEY)].as<int>();
        if (top[F(PIN_DC_KEY)].is<int>())      pinDc         = (int8_t)top[F(PIN_DC_KEY)].as<int>();
        if (top[F(PIN_RST_KEY)].is<int>())     pinRst        = (int8_t)top[F(PIN_RST_KEY)].as<int>();
        if (top[F(PIN_BL_KEY)].is<int>())      pinBl         = (int8_t)top[F(PIN_BL_KEY)].as<int>();
      }

      if (initDone) {
        #if (IS_ESPI_ACTIVE == 1)
          engine.init();
        #else
          engine.init(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
        #endif
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
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);
