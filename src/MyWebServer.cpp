#include "MyWebServer.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>

extern WebServer server;
extern Preferences preferences;
extern String version;
extern void update_tab1();

void web_request(float &Aanvoer, float &Afvoer) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://192.168.2.102/api/temperatuur");
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            JSONVar doc = JSON.parse(payload);
            if (JSON.typeof(doc) == "object") {
                Aanvoer = (double)doc["Aanvoer WP"];
                Afvoer = (double)doc["Afvoer WP"];
            }
        }
        http.end();
    }
}

String htmlPage() {
    String page = "<html><head><title>Version Update</title></head><body>";
    page += "<h2>Update Version</h2>";
    page += "<form method='POST' action='/set_version'>";
    page += "Version: <input name='version' value='" + version + "'>";
    page += "<input type='submit' value='Save'>";
    page += "</form>";
    page += "<p>Current version: " + version + "</p>";
    page += "</body></html>";
    return page;
}

void handleRoot() {
    server.send(200, "text/html", htmlPage());
}
