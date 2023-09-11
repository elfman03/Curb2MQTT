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
DWORD startTicks;
DWORD continueTicks;

// based on https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WinhttpWebsocket/cpp/WinhttpWebsocket.cpp
// https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md

void handleUTF8(const char *payload) {
  char outbuf[1024];
  DWORD ret;

  if(payload[0]=='4' && payload[1]=='2' && payload[2]=='/' && payload[22]=='d' && payload[23]=='a') {
    //
    // Short circuit for 42/api/circuit-data,["data",
    //                   0000000000111111111122222222
    //                   0123456789012345678901234567
    //
    DWORD tick=GetTickCount();
    printf("t=%0.1f circuit data payload\n",(tick-startTicks)/1000.0);
    //
    // ... UNDOCUMENTED IN THE CURB API ...
    // After about 70-80 seconds the server side tends to close the websocket with status websocket 1000 ("Normal Closure").
    // Kicking a simple socket.io packet into the mix every minute seems to keep the server side going for about 2 hours
    //
    if(tick>continueTicks) {
      myWS->postUTF8("42");
      continueTicks=tick+60000;
    }
  } else if(!strcmp(payload,"40/api/circuit-data")) {
    //
    // Initial 40/api/circuit-data was received by server. can now authenticate
    //
    printf("initializing... sending authentication...\n");
    sprintf(outbuf,"42/api/circuit-data,[\"authenticate\",{\"token\":\"%s\"}]",myToken->getAuthToken(myConfig, false));
    myWS->postUTF8(outbuf);
  } else if(!strcmp(payload,"42/api/circuit-data,[\"authorized\"]")) {
    //
    // Authentication was received by the server.  can now subscribe.
    //
    printf("Authorized!... sending zone registration\n");
    sprintf(outbuf,"42/api/circuit-data,[\"subscribe\",\"%s\"]",myConfig->getCurbUID());
    myWS->postUTF8(outbuf);
  } else {
    printf("UNKNOWN PAYLOAD: %s\n",payload);
  }
}

void main() {
  printf("Loading Config File\n");
  myConfig->readConfig("Curb2MQTT.config");
  printf("Retrieving Curb Access Token\n");
  const char *c=myToken->getAuthToken(myConfig, false);
  printf("Creating WebSocket\n");
  myWS->createWebSocket();
  startTicks=GetTickCount();
  continueTicks=startTicks+60000;
  myWS->postUTF8("40/api/circuit-data");
  myWS->looper(&handleUTF8);
}

