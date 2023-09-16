#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "global.h"
#include "Config.h"
#include "PahoWrapper.h"
#include "CircuitStateManager.h"

#define UNK 0
#define OFF 1
#define ON  2
static const char *circuitMsg[3]={"UNK","OFF","ON"};

CircuitStateManager::CircuitStateManager(Config *config, PahoWrapper *thePaho) {
  int i;

  paho=thePaho;
  logfile=config->getLogfile();
  for(i=0;i<8;i++) {
    const char *n=config->getCircuitName(i);
    if(n) {
      circuitName[i]=n;
      circuitLabel[i]=(char*)malloc(strlen(n)+20);
      sprintf(circuitLabel[i],"\"label\":\"%s\",",n);
    } else {
      circuitName[i]=0;
      circuitLabel[i]=0;
    }
    circuitThreshold[i]=config->getCircuitThreshold(i);
    circuitState[i]=circuitStateLast[i]=UNK;
    if(circuitName[i]) {
#ifdef DEBUG_PRINT_CSTATE
      if(logfile) { fprintf(logfile,"Circuit %d: '%s' -- threshold=%d\n",i,circuitName[i],circuitThreshold[i]); }
#endif
    }
  }
}

void CircuitStateManager::processDataPacket(const char *payload) {
  int i,watt;
  const char *p,*q;
#ifdef DEBUG_PRINT_CSTATE
  //if(logfile) { fprintf(logfile,"payload=%s\n",payload); }
#endif
  for(i=0;i<8;i++) {
    if(circuitLabel[i]) {
      p=strstr(payload,circuitLabel[i]);
      if(p) {
        for(p=p-4; (p>payload) && ((p[0]!='"') || (p[1]!='w') || (p[2]!='"') || (p[3]!=':')); ) {
          p=p-1;
        }
        if(p>payload) {
          watt=atoi(&p[4]);
          circuitStateLast[i]=circuitState[i];
          if(circuitThreshold[i]>=0) {
            //
            // for power consuming circuits, ON means watts is more than the threshold
            //
            if(watt>circuitThreshold[i]) { circuitState[i]=ON; } else { circuitState[i]=OFF; }
          } else {
            //
            // for power generation circuits (like solar panels, on means the watts is less than the threshold
            //
            if(watt<circuitThreshold[i]) { circuitState[i]=ON; } else { circuitState[i]=OFF; }
          }
          if(circuitStateLast[i]!=circuitState[i]) {
#ifdef DEBUG_PRINT_CSTATE
            if(logfile) { fprintf(logfile,"Circuit %3s -> %3s : Watts=%6d -- %s\n",
                                  circuitMsg[circuitStateLast[i]], circuitMsg[circuitState[i]], 
                                  watt, circuitName[i]); }
#endif
            paho->writeState(i, circuitMsg[circuitState[i]]);
          }
          //if(logfile) { fprintf(logfile,"%s: %d\n",circuitName[i],watt); }
        } else {
          fprintf(stderr,"ERROR: could not find \"w\": for %s in payload %s\n",circuitName[i],payload);
        }
      } else {
        fprintf(stderr,"ERROR: could not find %s in payload %s\n",circuitLabel[i],payload);
      }
    }
  }
}
