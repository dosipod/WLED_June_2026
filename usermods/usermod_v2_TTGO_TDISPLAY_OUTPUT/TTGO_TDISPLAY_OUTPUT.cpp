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
    
    int pinMosi = -1;
    int pinSclk = -1;
    int pinCs   = -1;
    int pinDc   = -1;
    int pinRst  = -1;
    int pinBl   = -1;

    bool initDone = false;
    bool lastPowerState = true;

    // VERIFIED WORKING KEY DECLARATIONS WITH BRACKETS
    const char SETTING_PROFILE[] = "Hardware-Profile";
    const char SETTING_WIDTH[]   = "Display-Width";
    const char SETTING_HEIGHT[]  = "Display-Height";
    const char SETTING_OFFSET[]  = "Column-Offset";
    const char PIN_MOSI_KEY[]    = "Pin-MOSI";
    const char PIN_SCLK_KEY[]    = "Pin-SCLK";
    const char PIN_CS_KEY[]      = "Pin-CS";
    const char PIN_DC_KEY[]      = "Pin-DC";
    const char PIN_RST_KEY[]     = "Pin-RST";
    const char PIN_BL_KEY[]      = "Pin-Backlight";

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
        displayWidth = TFT_WIDTH;
        displayHeight = TFT_HEIGHT;
        pinMosi = TFT_MOSI;
        pinSclk = TFT_SCLK;
        pinCs   = TFT_CS;
        pinDc   = TFT_DC;
        pinRst  = TFT_RST;
        pinBl   = TFT_BL;
        selectedProfile = 0;
      #else
        applyHardwareProfile();
      #endif
    }

    void init() override {
      if (initDone) return;

      if (pinMosi >= 0) {
        int8_t pinsToAllocate[] = { (int8_t)pinMosi, (int8_t)pinSclk, (int8_t)pinCs, (int8_t)pinDc, (int8_t)pinRst, (int8_t)pinBl };
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
      top[FPSTR(SETTING_PROFILE)] = selectedProfile; 
      top[FPSTR(SETTING_WIDTH)]  = displayWidth;
      top[FPSTR(SETTING_HEIGHT)] = displayHeight;
      top[FPSTR(SETTING_OFFSET)] = colOffset;
      top[FPSTR(PIN_MOSI_KEY)]   = pinMosi;
      top[FPSTR(PIN_SCLK_KEY)]   = pinSclk;
      top[FPSTR(PIN_CS_KEY)]     = pinCs;
      top[FPSTR(PIN_DC_KEY)]     = pinDc;
      top[FPSTR(PIN_RST_KEY)]    = pinRst;
      top[FPSTR(PIN_BL_KEY)]     = pinBl;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[F("DisplayMatrix")];
      if (top.isNull()) return false;

      int oldProfile = selectedProfile;
      
      if (top[FPSTR(SETTING_PROFILE)].is<int>()) {
        selectedProfile = top[FPSTR(SETTING_PROFILE)].as<int>();
      }

      if (selectedProfile != oldProfile && selectedProfile > 0) {
        applyHardwareProfile();
      } else {
        if (top[FPSTR(SETTING_WIDTH)].is<int>())   displayWidth  = top[FPSTR(SETTING_WIDTH)].as<int>();
        if (top[FPSTR(SETTING_HEIGHT)].is<int>())  displayHeight = top[FPSTR(SETTING_HEIGHT)].as<int>();
        if (top[FPSTR(SETTING_OFFSET)].is<int>())  colOffset     = top[FPSTR(SETTING_OFFSET)].as<int>();
        if (top[FPSTR(PIN_MOSI_KEY)].is<int>())    pinMosi       = top[FPSTR(PIN_MOSI_KEY)].as<int>();
        if (top[FPSTR(PIN_SCLK_KEY)].is<int>())    pinSclk       = top[FPSTR(PIN_SCLK_KEY)].as<int>();
        if (top[FPSTR(PIN_CS_KEY)].is<int>())      pinCs         = top[FPSTR(PIN_CS_KEY)].as<int>();
        if (top[FPSTR(PIN_DC_KEY)].is<int>())      pinDc         = top[FPSTR(PIN_DC_KEY)].as<int>();
        if (top[FPSTR(PIN_RST_KEY)].is<int>())     pinRst        = top[FPSTR(PIN_RST_KEY)].as<int>();
        if (top[FPSTR(PIN_BL_KEY)].is<int>())      pinBl         = top[FPSTR(PIN_BL_KEY)].as<int>();
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
