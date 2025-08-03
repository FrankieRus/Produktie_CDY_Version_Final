#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <Arduino_JSON.h>
#include <time.h>

extern WebServer server;

void web_request(float &Aanvoer, float &Afvoer);
void weather_request(float &temperature, float &humidity, float &pressure, float &bmp_temp, float &altitude);
String htmlPage();
void handleRoot();
bool getTimeFromAPI();
String getCurrentTime();
void setFallbackTime();

#endif // MYWEBSERVER_H
