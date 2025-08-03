#include <lvgl.h>
#include "System.h"
#include "UI/ui.h"
#include "procs.h"
#include <ArduinoOTA.h>
#include "MyWebServer.h"
// Versien 01
// Global variables
TFT_eSPI tft = TFT_eSPI(); 
SPIClass touchscreenSpi = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(33, 36); // CS, IRQ pins
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240,touchScreenMaximumY = 3800; 
lv_indev_t * indev; //Touchscreen input device
uint8_t* draw_buf;  //draw_buf is allocated on heap otherwise the static area is too big on ESP32 at compile
uint32_t lastTick = 0;  //Used to track the tick timer
extern void restore_active_tab();

void setup()
{
  bool usePortrait = false;  // Verander naar true voor portrait mode
  initializeSystem(usePortrait);
  lv_label_set_text(ui_version, "v1.0.0");

  restore_active_tab();
  lv_refr_now(NULL);

  lv_obj_add_event_cb(ui_TabView1, TabWissel, LV_EVENT_ALL, NULL);
  lv_label_set_text(ui_version, version.c_str());

}

// ...existing code...
unsigned long lastWebRequest = 0;
float aanvoer = 0, afvoer = 0;
float temperature = 0, humidity = 0, pressure = 0, bmp_temp = 0, altitude = 0;

void loop()
{   
    lv_tick_inc(millis() - lastTick); //Update the tick timer. Tick is new for LVGL 9
    lastTick = millis();
    lv_timer_handler();               //Update the UI
    webserver_loop();
    ArduinoOTA.handle();

    // Elke 30 seconden web_request en weather_request uitvoeren
    if (millis() - lastWebRequest > 30000) {
        web_request(aanvoer, afvoer);
        weather_request(temperature, humidity, pressure, bmp_temp, altitude);
        update_labels(aanvoer, afvoer, temperature, pressure);
        lastWebRequest = millis();
    }

    delay(1);
}


