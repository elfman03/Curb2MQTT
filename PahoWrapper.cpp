#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <MQTTClient.h>
#include "global.h"
#include "Config.h"
#include "PahoWrapper.h"

PahoWrapper::PahoWrapper(Config *config) {
  logfile=config->getLogfile();
  mqttServer=config->getMqttServer();
#ifdef DEBUG_PRINT_MQTT
  if(logfile) { fprintf(logfile,"MQTT Server: %s\n",mqttServer); }
#endif
  const char *base=config->getMqttTopicBase();
  for(int i=0;i<8;i++) {
    const char *name=config->getCircuitName(i);
    //
    if(name) {
      topicState[i]=(char*)malloc(strlen(base)+strlen(name)+20);
      sprintf(topicState[i],"%s%s/state",base,name);
      //
      topicAvailability[i]=(char*)malloc(strlen(base)+strlen(name)+20);
      sprintf(topicAvailability[i],"%s%s/availability",base,name);
#ifdef DEBUG_PRINT_MQTT
      if(logfile) {
        fprintf(logfile,"Circuit(%d): %s state=%s availability=%s\n",i,name,topicState[i],topicAvailability[i]);
      }
#endif
    } else {
      topicState[i]=0;
      topicAvailability[i]=0;
    }
  }
}

void PahoWrapper::writeMessage(const char *topic, const char *msg) {
#ifdef DEBUG_PRINT_MQTT
  if(logfile) { 
    fprintf(logfile,"Writing message '%s' to topic '%s'\n",msg, topic); 
  }
#endif
}

void PahoWrapper::writeToMQTT(int circuit, const char *msg) {
  writeMessage(topicState[circuit], msg);
}

void PahoWrapper::markAvailable(bool avail) {
  int i;
  for(i=0;i<8;i++) {
    if(topicAvailability[i]) {
      writeMessage(topicAvailability[i], avail?"Online":"Offline");
    }
  }
}
