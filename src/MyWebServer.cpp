#include "MyWebServer.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <time.h>

extern WebServer server;
extern Preferences preferences;
extern String version;
extern void update_tab1();

// Extern declaraties voor WiFi credentials uit System.cpp
extern const char* ssid;
extern const char* password;
extern IPAddress local_IP;

bool getTimeFromAPI() {
    Serial.println("Ophalen tijd via DHCP WiFi verbinding...");
    
    // Maak tijdelijke DHCP verbinding
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);  // DHCP mode
    
    unsigned long startAttemptTime = millis();
    const unsigned long wifiTimeout = 10000; // 10 seconden timeout
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("DHCP WiFi verbinding mislukt voor tijdsync");
        return false;
    }
    
    Serial.println("DHCP WiFi verbonden voor tijdsync: " + WiFi.localIP().toString());
    
    // Haal tijd op via API
    HTTPClient http;
    http.begin("http://worldtimeapi.org/api/timezone/Europe/Amsterdam");
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    bool success = false;
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Tijd API response ontvangen");
        
        // Parse JSON response voor datetime veld
        int datetimeIndex = payload.indexOf("\"datetime\":\"");
        if (datetimeIndex != -1) {
            datetimeIndex += 12; // Skip "datetime":"
            String datetime = payload.substring(datetimeIndex, datetimeIndex + 19);
            
            // Parse datum en tijd (format: 2024-08-02T14:30:45)
            int year = datetime.substring(0, 4).toInt();
            int month = datetime.substring(5, 7).toInt();
            int day = datetime.substring(8, 10).toInt();
            int hour = datetime.substring(11, 13).toInt();
            int minute = datetime.substring(14, 16).toInt();
            int second = datetime.substring(17, 19).toInt();
            
            if (year > 2020 && month >= 1 && month <= 12) {
                // Stel systeem tijd in
                struct tm timeinfo;
                timeinfo.tm_year = year - 1900;
                timeinfo.tm_mon = month - 1;
                timeinfo.tm_mday = day;
                timeinfo.tm_hour = hour;
                timeinfo.tm_min = minute;
                timeinfo.tm_sec = second;
                timeinfo.tm_isdst = -1;
                
                time_t t = mktime(&timeinfo);
                struct timeval tv = { .tv_sec = t };
                settimeofday(&tv, NULL);
                
                Serial.println("Tijd succesvol ingesteld: " + getCurrentTime());
                success = true;
            }
        }
    } else {
        Serial.println("Tijd API mislukt, HTTP code: " + String(httpCode));
    }
    
    http.end();
    
    // Sluit DHCP WiFi verbinding
    WiFi.disconnect(true);
    delay(1000);
    Serial.println("DHCP WiFi verbinding gesloten");
    
    return success;
}

String getCurrentTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Kan geen tijd verkrijgen");
        return "Tijd niet beschikbaar";
    }
    
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m/%Y %H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}

void setFallbackTime() {
    // Stel een redelijke standaardtijd in als backup (3 aug 2025, 12:00)
    struct tm timeinfo;
    timeinfo.tm_year = 2025 - 1900;
    timeinfo.tm_mon = 7; // Augustus (0-11)
    timeinfo.tm_mday = 3;
    timeinfo.tm_hour = 12;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    timeinfo.tm_isdst = 0;
    
    time_t t = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = t };
    settimeofday(&tv, NULL);
    
    Serial.println("Fallback tijd ingesteld: " + getCurrentTime());
}

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

void weather_request(float &temperature, float &humidity, float &pressure, float &bmp_temp, float &altitude) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://192.168.2.120/api");
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println("Weather API response: " + payload);
            JSONVar doc = JSON.parse(payload);
            if (JSON.typeof(doc) == "object") {
                temperature = (double)doc["temperature"];
                humidity = (double)doc["humidity"];
                pressure = (double)doc["pressure"];
                bmp_temp = (double)doc["bmp_temp"];
                altitude = (double)doc["altitude"];
                Serial.println("Weather data parsed successfully");
            } else {
                Serial.println("Weather JSON parse failed");
            }
        } else {
            Serial.println("Weather API failed, HTTP code: " + String(httpCode));
        }
        http.end();
    } else {
        Serial.println("No WiFi connection for weather request");
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
