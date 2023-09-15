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

void PahoWrapper::writeToMQTT(const char *topic, const char *msg) {
}
