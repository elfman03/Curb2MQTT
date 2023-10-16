/*
 * Copyright (C) 2023, Chris Elford
 * 
 * SPDX-License-Identifier: MIT
 *
 */

#include <windows.h>
#include <winhttp.h>
#include <time.h>
#include <stdio.h>
#include "global.h"
#include "Config.h"
#include "AuthToken.h"
#include "WebSocket.h"
#include "PahoWrapper.h"
#include "CircuitStateManager.h"

Config *myConfig=new Config();
AuthToken *myToken=new AuthToken();
WebSocket *myWS=new WebSocket();
CircuitStateManager *myStateMan;
PahoWrapper *myPaho;

DWORD firstTick;
DWORD continueTick;
DWORD availabilityTick;
FILE *logfile;
int epochNum=0;
int packetCount;
int packetCountEpoch;

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
int handleUTF8(const char *payload) {
  char outbuf[1024];
  int ret=0;

  DWORD tick=GetTickCount();
  if(!firstTick) { firstTick=tick; continueTick=tick+60000; availabilityTick=tick+(15*60*60000); }

  if(payload[0]=='4' && payload[1]=='2' && payload[2]=='/' && payload[22]=='d' && payload[23]=='a') {
    // Short circuit for 42/api/circuit-data,["data",
    //                   0000000000111111111122222222
    //                   0123456789012345678901234567
    //
    //
    // An actual data packet
    //
    // If its the first packet of the Epoch or its been 15 hours in the epoch, freshen the availability
    //
    if((packetCountEpoch==0) || (tick>availabilityTick)) {
      availabilityTick=tick+(15*60*60000);
      myPaho->markAvailable(true);
    }
    packetCount++;
    packetCountEpoch++;
#ifdef DEBUG_PRINT_MAIN
    //if(logfile) { fprintf(logfile,"t=%0.1f circuit data payload\n",(tick-firstTick)/1000.0); }
#endif
    //
    // ... UNDOCUMENTED IN THE CURB API ...
    // After about 70-80 seconds the server side tends to close the websocket with status websocket 1000 ("Normal Closure").
    // Kicking a simple socket.io packet into the mix every minute seems to keep the server side going for about 2 hours
    //
    if(tick>continueTick) {
      myWS->postUTF8("42");
      continueTick=tick+60000;
#ifdef DEBUG_PRINT_MAIN
      if(logfile) { 
        int h, m, s, frac;
        frac=(tick-firstTick);
        h=frac/(1000*60*60); frac=frac%(1000*60*60);
        m=frac/(1000*60);    frac=frac%(1000*60);
        s=frac/(1000);       frac=frac%(1000);
        frac=frac/100;
        int outstand=myPaho->getOutstanding();
        fprintf(logfile, "Stats: packetCount(60s)=%d packetCount(epoch)=%d epoch=%d epochTime=%d:%02d:%02d.%01d mqttOutstanding=%d\n",packetCount,packetCountEpoch,epochNum,h,m,s,frac,outstand);
        fflush(logfile); 
      }
#endif
      packetCount=0;
    }
    myStateMan->processDataPacket(payload);
  } else if(!strcmp(payload,"40/api/circuit-data")) {
    //
    // Initial 40/api/circuit-data was received by server. can now authenticate
    //
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { fprintf(logfile, "initializing... sending authentication...\n"); }
#endif
    sprintf(outbuf,"42/api/circuit-data,[\"authenticate\",{\"token\":\"%s\"}]",myToken->getAuthToken(myConfig, false));
    myWS->postUTF8(outbuf);
  } else if(!strcmp(payload,"42/api/circuit-data,[\"authorized\"]")) {
    //
    // Authentication was received by the server.  can now subscribe.
    //
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { fprintf(logfile, "Authorized!... sending zone registration\n"); }
#endif
    sprintf(outbuf,"42/api/circuit-data,[\"subscribe\",\"%s\"]",myConfig->getCurbUID());
    myWS->postUTF8(outbuf);
  } else {
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { fprintf(logfile, "UNHANDLED PAYLOAD: %s\n",payload); }
#endif
  }
  
  // If PAHO has gone down return 711711
  //
  if(!myPaho->isUp()) { ret=711711; }

  return ret;
}

void main() {
  int status;
  //
  // Load Config
  //
  fprintf(stderr, "Loading Config File\n");
  myConfig->readConfig("Curb2MQTT.config");
  logfile=myConfig->getLogfile();
  if(logfile) { fprintf(logfile, "Loaded Config File.  Creating CircuitStateManager\n"); }
  myPaho=new PahoWrapper(myConfig);
  myStateMan=new CircuitStateManager(myConfig, myPaho);
  myPaho->markAvailable(false);

  //
  // Loop as long as the websocket returns a normal exit condition.  Should be about 2 hours at a go.
  //
  for(status=1000;status==1000;) {
    //
    // If Paho connection is down, reconnect it.
    //
    if(!myPaho->isUp()) { myPaho->reconnect(); }

    firstTick=continueTick=availabilityTick=0;
    epochNum++;
    packetCount=packetCountEpoch=0;
    //
    // Fetch Access Token
    //
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { 
      time_t clock;
      time(&clock);
      fprintf(logfile,"Epoch %d begins: %s\n", epochNum, asctime(localtime(&clock)));
      fprintf(logfile, "Retrieving Curb Access Token\n"); 
   }
#endif
    const char *c=myToken->getAuthToken(myConfig, true);
    //
    // Create WebSocket and Trigger it
    //
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { fprintf(logfile,"Creating WebSocket\n"); }
#endif
    myWS->createWebSocket(AGENT, API_HOST, API_PATH, logfile);
    myWS->postUTF8("40/api/circuit-data");
    if(logfile) { fflush(logfile); }
    status=myWS->looper(&handleUTF8);
    //
    // Why did looper end?
    //
    DWORD tick=GetTickCount();
    if(!firstTick) { firstTick=tick; }
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { 
      int h, m, s, frac;
      frac=(tick-firstTick);
      h=frac/(1000*60*60); frac=frac%(1000*60*60);
      m=frac/(1000*60);    frac=frac%(1000*60);
      s=frac/(1000);       frac=frac%(1000);
      frac=frac/100;
      fprintf(logfile, "Looper ended with status %d (normal=1000) Epoch %d after epochTime=%d:%02d:%02d.%01d packets=%d\n",status,epochNum,h,m,s,frac,packetCountEpoch);
      fprintf(logfile, "tick=%ul firstTick=%ul\n",tick,firstTick);
      fflush(logfile); 
    }
#endif
    char *msg=0;
    int wait=0;
    //
    // mark to MQTT server that we are offline and mark locally that device states are unknown to resend when we come back
    //
    myPaho->markAvailable(false);
    myStateMan->unkState();

    // If we had an expected exit note it and restart after a bit.
    if(status==1000) { 
      wait=3; 
      msg="NORMAL_CLOSURE.  Start over in 3 minutes"; 
    } else if(status==12002) { 
      wait=5; 
      msg="ERROR_WINHTTP_TIMEOUT.  Try again in 5 mins";
      status=1000;
    } else if(status==12007) { 
      wait=5; 
      msg="ERROR_WINHTTP_NAME_NOT_RESOLVED.  Try again in 5 mins";
      status=1000;
    } else if(status==12030) { 
      wait=5; 
      msg="ERROR_WINHTTP_CONNECTION_ERROR.  Try again in 5 mins";
      status=1000;
    } else if(status==711711) {
      wait=1; 
      msg="ERROR_PAHO_DOWN.  Try again in 1 min";
      status=1000;
    }
#ifdef DEBUG_PRINT_MAIN
    if(logfile && msg) { fprintf(logfile, "%s\n",msg); }
#endif
    Sleep(wait*60*1000); 
  }
  if(logfile) { fflush(logfile); } // should be in Config.cpp destructor?
}

