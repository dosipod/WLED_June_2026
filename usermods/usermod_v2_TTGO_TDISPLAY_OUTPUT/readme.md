












# WLED T-Display Display Configurations

This usermod was tested on  the setup listed below. Configure your settings below based on your hardware version.

## 1. ESP32-S3 HMI 8M PSRAM 16M 1.9 " 170*320 Display  1.9 inch IPS LCD TFT Module
* **Resolution:** 320x170
* **WLED Matrix Size Setup:** `106x54`

### Code Configuration
```cpp
// S3 Setup Parameters ST7789_SIZE_320X170
const uint16_t targetWidth  = 320;
const uint16_t targetHeight = 170;
```
Full board info : ESP32-S3 HMI 8M PSRAM 16M Flash Arduino LVGL WIFI&Bluetooth 1.9 " 170*320 Smart Display Screen 1.9 inch IPS LCD TFT Module
https://ar.aliexpress.com/item/1005007610009494.html?spm=a2g0o.order_list.order_list_main.5.58da1802nM5M7J&gatewayAdapt=glo2ara
---

## 2. Classic TTGO T-Display ( or clone) 
* **Resolution:** 240x135
* **WLED Matrix Size Setup:** `80x44`

### Code Configuration
```cpp
// Fallback / Classic TTGO T-Display Setup Parameters ST7789_SIZE_240X135
const uint16_t targetWidth  = 240;
const uint16_t targetHeight = 135;
```
Full board info : T Display ESP32 WiFi And Bluetooth-Compatible Module Development Board 1.14 Inch LCD Control
https://ar.aliexpress.com/item/1005006495816339.html?spm=a2g0o.order_list.order_list_main.43.58da1802nM5M7J&gatewayAdapt=glo2ara
---
*Todo: Buy and add new display.*
