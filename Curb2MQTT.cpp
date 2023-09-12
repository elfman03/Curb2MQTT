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
const char *circuitNames[8];
char *circuitLabels[8];
int circuitThresholds[8];
FILE *logfile;

#define AGENT L"Curb2Mqtt/1.0"
/*
 *
 * API Constants from the Curb API
 *           https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md
 *
 */
#define API_HOST L"app.energycurb.com"
#define API_PATH L"/socket.io/?EIO=3&transport=websocket"

void processDataPacket(const char *payload) {
  int i,watt;
  const char *p,*q;
#ifdef DEBUG_PRINT_MAIN
  //if(logfile) { fprintf(logfile,"payload=%s\n",payload); }
#endif
  for(i=0;i<8;i++) {
    if(circuitLabels[i]) {
      p=strstr(payload,circuitLabels[i]);
      if(p) {
        for(p=p-4; (p>payload) && ((p[0]!='"') || (p[1]!='w') || (p[2]!='"') || (p[3]!=':')); ) {
          p=p-1;
        }
        if(p>payload) {
          watt=atoi(&p[4]);
#ifdef DEBUG_PRINT_MAIN
       if(logfile) { fprintf(logfile,"%s: %d\n",circuitNames[i],watt); }
#endif
        } else {
          fprintf(stderr,"ERROR: could not find \"w\": for %s in payload %s\n",circuitNames[i],payload);
        }
      } else {
        fprintf(stderr,"ERROR: could not find %s in payload %s\n",circuitLabels[i],payload);
      }
    }
  }
}

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
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { fprintf(logfile,"t=%0.1f circuit data payload\n",(tick-firstTick)/1000.0); }
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
      if(logfile) { fflush(logfile); }
#endif
    }
    processDataPacket(payload);
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
}

void main() {
  int status;
  //
  // Load Config
  //
  fprintf(stderr, "Loading Config File\n");
  myConfig->readConfig("Curb2MQTT.config");
  logfile=myConfig->getLogfile();
  if(logfile) { fprintf(logfile, "Loaded Config File\n"); }
  for(int i=0;i<8;i++) {
    const char *n=myConfig->getCircuitName(i);
    if(n) {
      circuitNames[i]=n;
      circuitLabels[i]=(char*)malloc(strlen(n)+20);
      sprintf(circuitLabels[i],"\"label\":\"%s\",",n);
    } else {
      circuitNames[i]=0;
      circuitLabels[i]=0;
    }
    circuitThresholds[i]=myConfig->getCircuitThreshold(i);
    if(circuitNames[i]) {
#ifdef DEBUG_PRINT_MAIN
      if(logfile) { fprintf(logfile,"Circuit %d: '%s' -- threshold=%d\n",i,circuitNames[i],circuitThresholds[i]); }
#endif
    }
  }

  //
  // Loop as long as the websocket returns a normal exit condition.  Should be about 2 hours at a go.
  //
  for(status=1000;status==1000;) {
    //
    firstTick=continueTick=0;
    //
    // Fetch Access Token
    //
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { fprintf(logfile, "Retrieving Curb Access Token\n"); }
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
    status=myWS->looper(&handleUTF8);
    //
    // Why did looper end?
    //
    DWORD tick=GetTickCount();
#ifdef DEBUG_PRINT_MAIN
    if(logfile) { 
       fprintf(logfile, "Looper ended with status %d (normal=1000) after %0.1f seconds\n",status,(tick-firstTick)/1000.0);
       fflush(logfile); 
    }
#endif
    // If we had a normal exit, wait 3 seconds before iterating
    if(status==1000) { Sleep(3000); } 
  }
  //if(logfile && logfile!=stderr && logfile!=stdout) { fclose(logfile); } // should be in Config.cpp destructor?
}

