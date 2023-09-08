#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "Curb2MQTT.h"
#include "Config.h"

Config *myConfig=new Config();

// Stuff to load from config file
//
const char *CURB_USERNAME;
const char *CURB_PASSWORD;
const char *CURB_CLIENT_ID;
const char *CURB_CLIENT_SECRET;

char *AUTH_BUF=0;
char *AUTH_CODE=0;

// based on https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WinhttpWebsocket/cpp/WinhttpWebsocket.cpp
// https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md

void main() {
  printf("Loading Config File\n");
  myConfig->readConfig("Curb2MQTT.config");
  CURB_USERNAME=myConfig->getCurbUsername();
  CURB_PASSWORD=myConfig->getCurbPassword();
  CURB_CLIENT_ID=myConfig->getCurbClientId();
  CURB_CLIENT_SECRET=myConfig->getCurbClientSecret();
  printf("Retrieving Curb Access Token\n");
  get_curb_token();
  printf("Creating WebSocket\n");
  create_websocket();
}

