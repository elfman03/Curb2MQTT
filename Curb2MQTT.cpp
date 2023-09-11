#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "global.h"
#include "Config.h"
#include "AuthToken.h"
#include "WebSocket.h"

Config *myConfig=new Config();
AuthToken *myToken=new AuthToken();
WebSocket *myWS=new WebSocket();

// based on https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WinhttpWebsocket/cpp/WinhttpWebsocket.cpp
// https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md

void main() {
  printf("Loading Config File\n");
  myConfig->readConfig("Curb2MQTT.config");
  printf("Retrieving Curb Access Token\n");
  const char *c=myToken->getAuthToken(myConfig, false);
  printf("Creating WebSocket\n");
  myWS->createWebSocket();
  myWS->registerForLive(c);
  myWS->looper();
}

