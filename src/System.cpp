
#include "System.h"
#include "../UI/ui.h"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Arduino_JSON.h>
#include <Preferences.h>
#include "MyWebServer.h"

// Vast IP adres van WIFI
IPAddress local_IP(192,168,2,118);


WebServer server(80);
Preferences preferences;
String version;

extern void update_tab1(); // Implementatie in deze file verderop

// Pin definitions for touchscreen
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Display resolution variables
uint16_t TFT_HOR_RES = 320;
uint16_t TFT_VER_RES = 240;
uint32_t DRAW_BUF_SIZE = 0;

const char* ssid = "ORBI";
const char* password = "marrrr222";

void wifi_init() {
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    const unsigned long wifiTimeout = 5000; // 5 seconden timeout
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
        delay(250);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi connect timeout, start zonder WiFi!");
    }
}



#include "WebServer.h"

void handleSetVersion() {
    if (server.hasArg("version")) {
        version = server.arg("version");
        preferences.putString("version", version); // Sla versie alleen op na HTML input
        update_tab1();
    }
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Updated. Redirecting...");
}

void webserver_init() {
    preferences.begin("my-app", false);
    version = preferences.getString("version", version);
    server.on("/", handleRoot);
    server.on("/set_version", HTTP_POST, handleSetVersion);
    server.begin();
    Serial.println("Webserver started");
    update_tab1(); // Initialiseer de tab met de huidige versie
}

void webserver_loop() {
    server.handleClient();
}

// Voorbeeld-implementatie van update_tab1 (pas aan naar je eigen LVGL structuur)
void update_tab1() {
    // Extern LVGL label object moet beschikbaar zijn, bijvoorbeeld:
    extern lv_obj_t* ui_version;
    lv_label_set_text(ui_version, version.c_str());
    Serial.print("Version updated to: ");
    Serial.println(version);
}


/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )px_map, w * h, true );
    tft.endWrite();

    /*Call it to tell LVGL you are ready*/
    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
  if(touchscreen.touched())
  {
    TS_Point p = touchscreen.getPoint();
    //Some very basic auto calibration so it doesn't go out of range
    if(p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
    if(p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
    if(p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
    if(p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;
    //Map this to the pixel position
    data->point.x = map(p.x,touchScreenMinimumX,touchScreenMaximumX,1,TFT_HOR_RES); /* Touchscreen X calibration */
    data->point.y = map(p.y,touchScreenMinimumY,touchScreenMaximumY,1,TFT_VER_RES); /* Touchscreen Y calibration */
    data->state = LV_INDEV_STATE_PRESSED;
    /*
    Serial.print("Touch x ");
    Serial.print(data->point.x);
    Serial.print(" y ");
    Serial.println(data->point.y);
    */
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void initializeSystem(bool portrait)
{
    Serial.begin(115200);
  // Set resolution based on orientation
  if (portrait) {
    TFT_HOR_RES = 240;
    TFT_VER_RES = 320;
  } else {
    TFT_HOR_RES = 320;
    TFT_VER_RES = 240;
  }
  
  // Calculate draw buffer size
  DRAW_BUF_SIZE = (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8));

  // Init display en touchscreen direct
  tft.init();
  tft.setRotation(3); /* Landscape orientation, same as touchscreen */
  tft.fillScreen(TFT_BLACK);
  touchscreenSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); /* Start second SPI bus for touchscreen */
  touchscreen.begin(touchscreenSpi); /* Touchscreen init */
  touchscreen.setRotation(3); /* Inverted landscape orientation to match screen */

  //Initialiseer LVGL vóór WiFi/webserver
  lv_init();

  // Set rotation based on portrait/landscape choice
  if (portrait) {
    tft.setRotation(0);
    touchscreen.setRotation(0);
  } else {
    tft.setRotation(3);
    touchscreen.setRotation(3);
  }

  draw_buf = new uint8_t[DRAW_BUF_SIZE];
  static lv_display_t * disp;
  disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

  //Initialize the XPT2046 input device driver
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);  
  lv_indev_set_read_cb(indev, my_touchpad_read);

  // UI direct tonen
  ui_init();
  lv_timer_handler(); // Forceer eerste refresh

  // Daarna pas WiFi en webserver
  IPAddress gateway(192,168,2,1);
  IPAddress subnet(255,255,255,0);
  IPAddress primaryDNS(8,8,8,8);     // Google DNS
  IPAddress secondaryDNS(8,8,4,4);   // Google DNS backup
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  wifi_init();
  // Initialiseer NTP tijd synchronisatie na WiFi verbinding en haal tijd éénmalig op
  if (WiFi.status() == WL_CONNECTED) {
    initializeNTP();
    updateTimeFromInternet();
  }
  ArduinoOTA.setHostname("esp32-ota");
  ArduinoOTA.begin();
  webserver_init();
}
