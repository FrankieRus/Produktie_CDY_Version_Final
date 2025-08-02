#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <Arduino_JSON.h>

extern WebServer server;

void web_request(float &Aanvoer, float &Afvoer);
String htmlPage();
void handleRoot();

#endif // MYWEBSERVER_H
