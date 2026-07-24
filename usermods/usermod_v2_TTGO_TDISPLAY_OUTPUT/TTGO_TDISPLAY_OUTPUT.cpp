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

#ifdef USER_SETUP_LOADED
  #include "lcd_as_output_espi_engine.h"
#else
  #include "lcd_as_output_gfx_engine.h"
#endif

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    #ifdef USER_SETUP_LOADED
      LcdTfteSpiEngine engine;
    #else
      LcdGfxEngine engine;
    #endif

    // Profile Definitions: 0 = Custom/Manual, 1 = ESP32-S3-1732S019 Profile
    int selectedProfile = 1; 

    // Target Display settings parameters
    int displayWidth = 320;
    int displayHeight = 170;
    int colOffset = 35;
    
    // Extracted Hardware Bus Control Pins
    int pinMosi = -1;
    int pinSclk = -1;
    int pinCs   = -1;
    int pinDc   = -1;
    int pinRst  = -1;
    int pinBl   = -1;

    bool initDone = false;
    bool lastPowerState = true;

    // Fixed constant strings mapping configuration JSON nodes
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

    // Re-apply preset variables instantly when a profile is picked from the web dropdown menu
    void applyHardwareProfile() {
      if (selectedProfile == 1) { // ESP32-1732S019 HMI 1.9" Variant Setup Parameters
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
      // Step 1: Check if compiler overrides exist inside platformio.ini; use those as defaults first
      #ifdef TFT_WIDTH
        displayWidth = TFT_WIDTH;
        displayHeight = TFT_HEIGHT;
        pinMosi = TFT_MOSI;
        pinSclk = TFT_SCLK;
        pinCs   = TFT_CS;
        pinDc   = TFT_DC;
        pinRst  = TFT_RST;
        pinBl   = TFT_BL;
        selectedProfile = 0; // Set to custom if compile-time definitions override it
      #else
        // If compiling blindly on an automated test environment, drop down to the preset
        applyHardwareProfile();
      #endif

      // Step 2: Register pins with WLED's PinManager database
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

      // Step 3: Trigger active execution engine
      #ifdef USER_SETUP_LOADED
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

    // Pushes configuration values to web page inputs, generating an automated selection dropdown
    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(F("DisplayMatrix"));
      
      // Adding a numeric configuration node creates a native option dropdown selection menu in WLED
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
      selectedProfile = top[FPSTR(SETTING_PROFILE)] | selectedProfile;

      // If a user selects a pre-made profile from the option dropdown menu, instantly overwrite the parameters fields
      if (selectedProfile != oldProfile && selectedProfile > 0) {
        applyHardwareProfile();
      } else {
        // Otherwise, pull raw user modification changes from the custom inputs fields
        displayWidth  = top[FPSTR(SETTING_WIDTH)]  | displayWidth;
        displayHeight = top[FPSTR(SETTING_HEIGHT)] | displayHeight;
        colOffset     = top[FPSTR(SETTING_OFFSET)] | colOffset;
        pinMosi       = top[FPSTR(PIN_MOSI_KEY)]   | pinMosi;
        pinSclk       = top[FPSTR(PIN_SCLK_KEY)]   | pinSclk;
        pinCs         = top[FPSTR(PIN_CS_KEY)]     | pinCs;
        pinDc         = top[FPSTR(PIN_DC_KEY)]     | pinDc;
        pinRst        = top[FPSTR(PIN_RST_KEY)]    | pinRst;
        pinBl         = top[FPSTR(PIN_BL_KEY)]     | pinBl;
      }

      // Re-initialize drivers on-the-fly to handle live config tweaks smoothly over Wi-Fi
      if (initDone) {
        #ifdef USER_SETUP_LOADED
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
      top["profile"] = selectedProfile;
    }

    uint16_t getId() override {
      return USERMOD_ID_TTGO_TDISPLAY_OUTPUT;
    }
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);
