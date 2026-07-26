#include "wled.h"
#include "display_wrapper.h"

class TTGO_TDISPLAY_OUTPUT : public Usermod {
  private:
    DisplayWrapper* hw = nullptr;

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

    void initializeHardware() {
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

      hw = new DisplayWrapper();
      if (hw) {
        hw->initDriver(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
      }

      initDone = true;
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
    }

    void connected() override {
      initializeHardware();
    }

    void handleOverlayDraw() override {
      if (!initDone || !lastPowerState || !hw) return;
      hw->renderFrame(strip);
    }

    void loop() override {
      // ASYNCHRONOUS DECOUPLED INITIALIZATION LOCK:
      // Postpones raw hardware initialization until WLED confirms its system systems 
      // and internal wireless sockets are up and running, preventing any network starvation.
      if (!initDone && interfacesInited) {
        initializeHardware();
      }

      if (!initDone || !hw) return;

      bool currentPowerState = (bri > 0);
      if (currentPowerState != lastPowerState) {
        lastPowerState = currentPowerState;
        if (pinBl >= 0) digitalWrite(pinBl, lastPowerState ? HIGH : LOW);
        if (!lastPowerState && hw) {
          hw->clearScreen();
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
        if (hw) {
          delete hw;
          hw = nullptr;
        }
        initDone = false; 
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

        if (initDone && hw) {
          hw->initDriver(pinMosi, pinSclk, pinCs, pinDc, pinRst, displayWidth, displayHeight, colOffset);
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
      if (hw) {
        delete hw;
        hw = nullptr;
      }
    }
};

static TTGO_TDISPLAY_OUTPUT ttgo_display_mod;
REGISTER_USERMOD(ttgo_display_mod);
