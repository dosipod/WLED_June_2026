/*
  -----------------------------------------------------------------------------------------------
  Lilygo/TTGO T-Display screen output
  -----------------------------------------------------------------------------------------------

  Ideas/todos:
    - add rotation support
    - choose between square and nonsquare pixels (nonsquare fills the screen, square goes to center)
    - add reserved pins (tft) to xml.cpp
    - usermod configuration instead of led and 2D settings ?
    - add "display" as led type 
*/

#pragma once

#include "wled.h"
#include <TFT_eSPI.h>

#define ADC_EN 14  // Used for enabling battery voltage measurements - not used in this program
// #define measureRenderTime

// Explicit unique structural identification layout for this custom extension
#ifndef USERMOD_ID_TTGO_TDISPLAY_OUTPUT
#define USERMOD_ID_TTGO_TDISPLAY_OUTPUT 0x9D5F
#endif

class TTGOTDisplayOutputUsermod : public Usermod {
  private:
    TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); // Setup display

    uint8_t backlightChannel = 255;
    uint16_t blocksW = 0;
    uint16_t blocksH = 0;
    bool forceSquareBlocks = true;
    uint16_t top = 0;
    uint16_t left = 0;
    uint16_t oneWidth = 0;
    uint16_t oneHeight = 0;
    uint16_t margin = 1;
    bool enabled = true; // Required config runtime toggle flag

#ifdef measureRenderTime
    uint32_t rtSum     = 0;
    uint32_t rtSamples = 0;
#endif

    bool isSettingsValid(){
      if(!strip.isMatrix) return false;
      return true;
    }

    bool isSettingsChanged(){
      Segment& mainSeg = strip.getFirstSelectedSeg();
      // Emulating older direct layout flags through modern matrix segmentation rules
      uint16_t currentMargin = mainSeg.options & SEG_OPTION_REVERSED ? 0 : 1; 
      if(currentMargin != margin) return true;
      if(blocksW != mainSeg.virtualWidth()) return true;
      if(blocksH != mainSeg.virtualHeight()) return true;
      return false;
    }

    bool checkSettings(){
      if(!isSettingsValid()) return false;
      if(!isSettingsChanged()) return true;

      tft.fillScreen(TFT_BLACK); // clear screen on change

      Segment& mainSeg = strip.getFirstSelectedSeg();
      blocksW = mainSeg.virtualWidth();
      blocksH = mainSeg.virtualHeight();
      margin  = mainSeg.options & SEG_OPTION_REVERSED ? 0 : 1;

      oneWidth  = floor((tft.width()  - (blocksW-1)*margin) / blocksW);
      oneHeight = floor((tft.height() - (blocksH-1)*margin) / blocksH);
      if(forceSquareBlocks){
        oneHeight = oneWidth = min(oneWidth, oneHeight);
      }
      top  = (tft.height() - (blocksH-1)*margin - oneHeight * blocksH) / 2;
      left = (tft.width()  - (blocksW-1)*margin - oneWidth  * blocksW) / 2;

      return true;
    }

    void setBrightness(){
      if(!enabled) {
        digitalWrite(TFT_BL, LOW);
        return;
      }
      
      if(backlightChannel == 255){
        digitalWrite(TFT_BL, bri == 0 ? LOW : HIGH);
      }else{
#if defined(ESP_ARDUINO_VERSION_VAL) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(TFT_BL, bri); // Modern Core 3 API handles mapping natively 
#else
        ledcWrite(backlightChannel, bri);
#endif
      }
    }

  public:

    inline uint16_t getId() override {
      return USERMOD_ID_TTGO_TDISPLAY_MATRIX;
    }

    void setup() override {
      // Safely register display illumination line with central pin tracking engine
#ifdef TFT_BL
      if (!pinManager.allocatePin(TFT_BL, true, PinOwner::UM_Display)) {
        Serial.println(F("TTGO T-Display Matrix Usermod: Failed to allocate backlight pin."));
      }
#endif

      tft.init();
      tft.setRotation(3);  // Rotation here is set up for the text to be readable with the port on the left.
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.setTextDatum(MC_DATUM);

      setupBrightnessControl();
      setBrightness();

      uint8_t textSize = 4;
      while (true)
      {
        tft.setTextSize(textSize);
        if(textSize==1) break;
        if(tft.textWidth(F("WLED - DisplayMatrix"))<=tft.width()) break;
        textSize--;        
      }
      tft.drawString(F("WLED - DisplayMatrix"), tft.width()/2, tft.height()/2);
    }

    void setupBrightnessControl(){
      backlightChannel = pinManager.allocateLedc(1);
      if(backlightChannel == 255){
        pinMode(TFT_BL, OUTPUT);
      }else{
        // Modernized architecture targeting updated cross-platform framework channels
#if defined(ESP_ARDUINO_VERSION_VAL) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcAttach(TFT_BL, 10000, 8); 
#else
        ledcSetup(backlightChannel, 10000, 8);
        ledcAttachPin(TFT_BL, backlightChannel);
#endif
      }
    }

    void onStateChange(uint8_t mode) override {
      setBrightness();
    }

    void loop() override {
      if(!enabled || bri==0) return;
      if(!checkSettings()) return;

#ifdef measureRenderTime
      unsigned long rtStart = micros();
#endif

      for(uint16_t h = 0; h<blocksH; h++){
        for(uint16_t w = 0; w<blocksW; w++){
          tft.fillRect(left + w * (oneWidth + margin), top + h * (oneHeight + margin), oneWidth, oneHeight, tft.color24to16(strip.getPixelColorXY(w,h)));
        }
      }

#ifdef measureRenderTime
      if(micros() > rtStart){
        rtSum += micros() - rtStart;
        rtSamples++;
        if(rtSamples==100){
          Serial.printf("2D Matrix: %dx%d with %d gap, display: %lu microseconds\n", blocksW, blocksH, margin, rtSum / rtSamples);
          rtSamples = 0;
          rtSum = 0;
        }
      }
#endif
    }

    // Modern structural UI adjustments to prevent layout engine validation errors
    void addToConfig(JsonObject& root) override {
      JsonObject topObject = root.createNestedObject(F("ttgo_tdisplay_matrix"));
      topObject[F("enabled")] = enabled;
      topObject[F("forceSquare")] = forceSquareBlocks;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject topObject = root[F("ttgo_tdisplay_matrix")];
      if (topObject.isNull()) return false;

      enabled = topObject[F("enabled")] | enabled;
      forceSquareBlocks = topObject[F("forceSquare")] | forceSquareBlocks;
      return true;
    }
};


static TTGO_TDISPLAY_OUTPUT;
REGISTER_USERMOD(TTGO_TDISPLAY_OUTPUT);
