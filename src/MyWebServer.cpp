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
    
    // Probeer meerdere keren tijd op te halen voor betere betrouwbaarheid
    bool success = false;
    int attempts = 0;
    const int maxAttempts = 3;
    
    while (!success && attempts < maxAttempts) {
        attempts++;
        Serial.println("Tijdsync poging " + String(attempts) + " van " + String(maxAttempts));
        
        HTTPClient http;
        http.begin("http://worldclockapi.com/api/json/cet/now");
        http.setTimeout(8000); // Langere timeout
        
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println("Tijd API response ontvangen");
            Serial.println("Response: " + payload.substring(0, 200)); // Debug output
            
            // Parse JSON response - worldclockapi gebruikt "currentDateTime"
            int datetimeIndex = payload.indexOf("\"currentDateTime\":\"");
            if (datetimeIndex == -1) {
                // Fallback naar andere mogelijke velden
                datetimeIndex = payload.indexOf("\"datetime\":\"");
                if (datetimeIndex != -1) {
                    datetimeIndex += 12; // Skip "datetime":"
                }
            } else {
                datetimeIndex += 18; // Skip "currentDateTime":"
            }
            
            if (datetimeIndex != -1) {
                String datetime = payload.substring(datetimeIndex, datetimeIndex + 30); // Langere string voor parsing
                Serial.println("Gevonden datetime string: " + datetime);
                
                // Zoek het einde van de datetime waarde (tot " of , of })
                int endIndex = datetime.indexOf('"');
                if (endIndex == -1) endIndex = datetime.indexOf(',');
                if (endIndex == -1) endIndex = datetime.indexOf('}');
                if (endIndex > 0) {
                    datetime = datetime.substring(0, endIndex);
                }
                
                // Verwijder eventuele aanhalingstekens aan het begin
                if (datetime.startsWith("\"")) {
                    datetime = datetime.substring(1);
                }
                
                Serial.println("Schone datetime string: " + datetime);
                
                // Parse datum en tijd (format: 2025-08-03T15:33+02:00)
                if (datetime.length() >= 16 && datetime.indexOf('T') > 0) {
                    int year = datetime.substring(0, 4).toInt();
                    int month = datetime.substring(5, 7).toInt();
                    int day = datetime.substring(8, 10).toInt();
                    int hour = datetime.substring(11, 13).toInt();
                    int minute = datetime.substring(14, 16).toInt();
                    
                    // Controleer of er seconden zijn (karakter op positie 16 is ':' of '+')
                    int second = 0;
                    if (datetime.length() > 16 && datetime.charAt(16) == ':') {
                        second = datetime.substring(17, 19).toInt();
                    } else {
                        second = 0; // Geen seconden in de string
                    }
                    
                    Serial.println("Geparseerd: " + String(year) + "-" + String(month) + "-" + String(day) + " " + String(hour) + ":" + String(minute) + ":" + String(second));
                    
                    if (year > 2020 && month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour >= 0 && hour <= 23) {
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
                    } else {
                        Serial.println("Ongeldige datum ontvangen");
                    }
                } else {
                    Serial.println("Datetime string te kort");
                }
            } else {
                Serial.println("Kan datetime niet vinden in response");
            }
        } else {
            Serial.println("Tijd API mislukt, HTTP code: " + String(httpCode));
        }
        
        http.end();
        
        if (!success && attempts < maxAttempts) {
            Serial.println("Wacht 2 seconden voor volgende poging...");
            delay(2000);
        }
    }
    
    if (!success) {
        Serial.println("Alle tijdsync pogingen mislukt na " + String(maxAttempts) + " pogingen");
    }
    
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
