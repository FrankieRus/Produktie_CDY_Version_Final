#ifndef SYSTEM_H
#define SYSTEM_H

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>

// External variables from main.cpp
extern TFT_eSPI tft;
extern XPT2046_Touchscreen touchscreen;
extern SPIClass touchscreenSpi;
extern lv_indev_t * indev;
extern uint8_t* draw_buf;

// Touch screen calibration values
extern uint16_t touchScreenMinimumX, touchScreenMaximumX, touchScreenMinimumY, touchScreenMaximumY;

// Display configuration - will be set dynamically
extern uint16_t TFT_HOR_RES;
extern uint16_t TFT_VER_RES;
extern uint32_t DRAW_BUF_SIZE;
extern String version;

// Function declarations
void wifi_init();
void webserver_init();
void webserver_loop();
void initializeSystem(bool portrait = false);
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t * px_map);
void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);

#endif // SYSTEM_H
