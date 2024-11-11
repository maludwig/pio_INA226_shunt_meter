
#pragma once

#include <stdlib.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <dirent.h>
#include <stdio.h>

#include <string>
#include "sd_functions.h"
#include "ArduinoJson.h"
#include "server.h"


void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        Serial.printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < info->len; i++){
          Serial.printf("%02x ", data[i]);
        }
        Serial.printf("\n");
      }
      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        Serial.printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < len; i++){
          Serial.printf("%02x ", data[i]);
        }
        Serial.printf("\n");
      }

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

File upload_file;
size_t upload_file_size;
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  // request->pathArg(0) has the dir path on the sd card
  if(!index){
    //get specific header by name
    if(request->hasHeader("Content-Length")){
      AsyncWebHeader* h = request->getHeader("Content-Length");
      upload_file_size = atoi(h->value().c_str());
      dual_log("Content-Length: %u\n", upload_file_size);
    }
    char path[128];
    String dir_path = request->pathArg(0);
    dual_log("dirpath: %s", dir_path.c_str());
    // If the path is empty, then we are uploading to the root of the SD card
    if (dir_path.length() == 0) {
      sprintf(path, "/%s", filename.c_str());
    } else {
      sprintf(path, "/%s/%s", dir_path.c_str(), filename.c_str());
    }
    dual_log("UploadStart: %s, size (B): %u, writing to: %s", filename.c_str(), upload_file_size, path);
    upload_file = SD.open(path, FILE_WRITE);
    if (!upload_file) {
      dual_log("Failed to open file for writing");
      return;
    }
  }
  dual_log("Uploading: %s, %u / %u", filename.c_str(), index, upload_file_size);
  if (upload_file.write(data, len) != len) {
    dual_log("Write failed");
  }
  if(final){
    upload_file.flush();
    upload_file.close();
    dual_log("UploadEnd: %s, %u B", filename.c_str(), index+len);
  }
}

void onNotFoundRequest(AsyncWebServerRequest *request){
  if (request->method() == HTTP_OPTIONS) {
    request->send(200);
  } else {
    request->send(404);
  }
}

void setupWebHandlers() {

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  // Set up webserver
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/index.html", "text/html");
  });

  // upload a file to /upload
  server.on("^\\/api\\/upload\\/(.*)$", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200);
  }, handleUpload);

  server.on("^\\/api\\/list\\/(.*)$", HTTP_GET, [](AsyncWebServerRequest *request){

    String dirPath = "/sd/" + request->pathArg(0);
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->addHeader("Content-Type","application/json");

    // List files on the SD card at the path
    DIR *dir;
    struct dirent *entry;

    // Remove a trailing slash, if it exists
    if (dirPath.endsWith("/")) {
      dirPath = dirPath.substring(0, dirPath.length() - 1);
    }


    //List all parameters
    bool show_hidden = false;
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if (p->name() == "hidden") {
        if (p->value() == "true") {
          show_hidden = true;
        }
      }
    }

    // List directory contents
    dir = opendir(dirPath.c_str());
    if (!dir) {
        printf("Failed to open directory %s\n", dirPath);
        return;
    }
    response->print("[\"./\", \"../\"");
    while ((entry = readdir(dir)) != NULL) {
      // If hidden (begins with .), skip
      if (entry->d_name[0] == '.' && !show_hidden) {
        continue;
      }
      if (entry->d_type == DT_DIR) {
        response->printf(",\n\"%s/\"", entry->d_name);
      } else if (entry->d_type == DT_REG) {
        response->printf(",\n\"%s\"", entry->d_name);
      }
    }
    response->print("]");
    closedir(dir);
    request->send(response);
  });

  server.serveStatic("/www", SD, "/www");
  server.serveStatic("/daily", SD, "/daily");
  server.serveStatic("/full", SD, "/full");

  server.onNotFound(onNotFoundRequest);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();

}

