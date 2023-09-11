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
DWORD firstTick;
DWORD continueTick;

#define AGENT L"Curb2Mqtt/1.0"
/*
 *
 * API Constants from the Curb API
 *           https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md
 *
 */
#define API_HOST L"app.energycurb.com"
#define API_PATH L"/socket.io/?EIO=3&transport=websocket"

//
// See https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md
// Handle incoming websocket payload from Curb
//
void handleUTF8(const char *payload) {
  char outbuf[1024];
  DWORD ret;

  if(payload[0]=='4' && payload[1]=='2' && payload[2]=='/' && payload[22]=='d' && payload[23]=='a') {
    // Short circuit for 42/api/circuit-data,["data",
    //                   0000000000111111111122222222
    //                   0123456789012345678901234567
    //
    //
    // An actual data packet
    //
    DWORD tick=GetTickCount();
    if(!firstTick) { firstTick=tick; continueTick=tick+60000; }
    printf("t=%0.1f circuit data payload\n",(tick-firstTick)/1000.0);
    //
    // ... UNDOCUMENTED IN THE CURB API ...
    // After about 70-80 seconds the server side tends to close the websocket with status websocket 1000 ("Normal Closure").
    // Kicking a simple socket.io packet into the mix every minute seems to keep the server side going for about 2 hours
    //
    if(tick>continueTick) {
      myWS->postUTF8("42");
      continueTick=tick+60000;
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
    printf("UHANDLED PAYLOAD: %s\n",payload);
  }
}

void main() {
  int ret;
  //
  // Load Config
  //
  printf("Loading Config File\n");
  myConfig->readConfig("Curb2MQTT.config");

  //
  // Loop as long as the websocket returns a normal exit condition.  Should be about 2 hours at a go.
  //
  for(ret=1000;ret==1000;) {
    //
    firstTick=continueTick=0;
    //
    // Fetch Access Token
    //
    printf("Retrieving Curb Access Token\n");
    const char *c=myToken->getAuthToken(myConfig, true);
    //
    // Create WebSocket and Trigger it
    //
    printf("Creating WebSocket\n");
    myWS->createWebSocket(AGENT, API_HOST, API_PATH);
    myWS->postUTF8("40/api/circuit-data");
    ret=myWS->looper(&handleUTF8);
    //
    // Why did looper end?
    //
    DWORD tick=GetTickCount();
    printf("Looper ended with status %d (normal=1000) after %0.1f seconds\n",ret,(tick-firstTick)/1000.0);
    // If we had a normal exit, wait 3 seconds before iterating
    if(ret==1000) { Sleep(3000); } 
  }
}

