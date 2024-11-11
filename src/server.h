
#pragma once

#include <Arduino.h>
#include <stdarg.h>
#include <string>
#include <stdio.h>
#include <dirent.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "sd_functions.h"
#include "ArduinoJson.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

AsyncWebSocket ws("/ws");

DynamicJsonDocument doc(1024);

void dual_log(const char *format, ...) {

    char buffer[128];
    
    // Format the message
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    doc["message"] = buffer;
    doc["type"] = "LOG_MESSAGE";
    String json;
    serializeJson(doc, json);
    ws.printfAll(json.c_str());
    Serial.println(json.c_str());
    doc.clear();
}