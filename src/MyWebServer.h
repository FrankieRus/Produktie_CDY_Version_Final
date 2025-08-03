#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <Arduino_JSON.h>
#include <time.h>

extern WebServer server;

void web_request(float &Aanvoer, float &Afvoer);
String htmlPage();
void handleRoot();
bool getTimeFromAPI();
String getCurrentTime();
void setFallbackTime();

#endif // MYWEBSERVER_H
