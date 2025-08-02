#include "MyWebServer.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <time.h>
#include "lwip/apps/sntp.h"

extern WebServer server;
extern Preferences preferences;
extern String version;
extern void update_tab1();

// NTP server configuratie
const char* ntpServer = "time1.google.com";
const long gmtOffset_sec = 3600;          // GMT+1 (Nederland)
const int daylightOffset_sec = 3600;     // Zomertijd offset

void initializeNTP() {
    // Configureer tijd synchronisatie
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("NTP tijd synchronisatie gestart...");
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

void updateTimeFromInternet() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Tijd wordt bijgewerkt van internet...");
        Serial.print("WiFi IP: "); Serial.println(WiFi.localIP());
        Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP());
        Serial.print("DNS: "); Serial.println(WiFi.dnsIP());
        
        // Gebruik alleen NTP met IP-adressen (geen DNS)
        // Google NTP servers: 216.239.35.0, 216.239.35.4, 216.239.35.8, 216.239.35.12
        Serial.println("Configureer NTP met IP-adressen...");
        
        // Stop oude NTP en start opnieuw
        sntp_stop();
        delay(100);
        
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "216.239.35.0");  // time1.google.com IP
        sntp_setservername(1, "216.239.35.4");  // time2.google.com IP
        sntp_init();
        
        Serial.println("Wacht op NTP synchronisatie...");
        
        // Wacht maximaal 10 seconden
        struct tm timeinfo;
        int attempts = 0;
        const int maxAttempts = 20; // 20x 500ms = 10s
        
        while (!getLocalTime(&timeinfo) && attempts < maxAttempts) {
            Serial.print(".");
            delay(500);
            attempts++;
        }
        Serial.println();
        
        if (attempts < maxAttempts && timeinfo.tm_year > (2020-1900)) {
            Serial.println("NTP tijd succesvol bijgewerkt: " + getCurrentTime());
        } else {
            Serial.println("NTP synchronisatie mislukt. Handmatig een standaard tijd instellen...");
            
            // Stel handmatig een redelijke tijd in als backup (bijv. 1 jan 2025, 12:00)
            timeinfo.tm_year = 2025 - 1900;
            timeinfo.tm_mon = 0;  // Januari
            timeinfo.tm_mday = 1;
            timeinfo.tm_hour = 12;
            timeinfo.tm_min = 0;
            timeinfo.tm_sec = 0;
            timeinfo.tm_isdst = 0;
            
            time_t t = mktime(&timeinfo);
            struct timeval tv = { .tv_sec = t };
            settimeofday(&tv, NULL);
            
            Serial.println("Backup tijd ingesteld: " + getCurrentTime());
        }
    } else {
        Serial.println("Geen WiFi verbinding voor tijd synchronisatie");
    }
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
