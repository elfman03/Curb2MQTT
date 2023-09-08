#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "Curb2MQTT.h"

// Stuff to load from config file
//
char *CURB_USERNAME;
char *CURB_PASSWORD;
char *CURB_CLIENT_ID;
char *CURB_CLIENT_SECRET;

char *AUTH_BUF=0;
char *AUTH_CODE=0;

// based on https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WinhttpWebsocket/cpp/WinhttpWebsocket.cpp
// https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md

void main() {
  printf("Loading Config File\n");
  read_config();
  printf("Retrieving Curb Access Token\n");
  get_curb_token();
  printf("Creating WebSocket\n");
  create_websocket();
}

