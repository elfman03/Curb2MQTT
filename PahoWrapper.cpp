#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <MQTTAsync.h>
#include "global.h"
#include "Config.h"
#include "PahoWrapper.h"

extern "C" {

  static void _pahoOnConnLost(void *context, char *cause);
  static void _pahoOnConnectFailure(void* context, MQTTAsync_failureData* response);
  static void _pahoOnConnect(void* context, MQTTAsync_successData* response);
  static void _pahoOnSend(void* context, MQTTAsync_successData* response);
  static void _pahoOnSendFailure(void* context, MQTTAsync_failureData* response);

}

PahoWrapper::PahoWrapper(Config *config) {
  logfile=config->getLogfile();
  mqttServer=config->getMqttServer();
  mqttSetup();
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

void PahoWrapper::sendToPaho(const char *topic, const char *msg) {
#ifdef DEBUG_PRINT_MQTT
  if(logfile) { 
    fprintf(logfile,"Writing message '%s' to topic '%s'\n",msg, topic); 
  }
#endif

  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
  int rc;
  opts.onSuccess = _pahoOnSend;
  opts.onFailure = _pahoOnSendFailure;
  opts.context = (void*)this;
  pubmsg.payload = (void*)msg;
  pubmsg.payloadlen = strlen(msg);
  pubmsg.qos = 1;
  pubmsg.retained = 0;
  if ((rc = MQTTAsync_sendMessage(pahoClient, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
    fprintf(stderr,"PAHO_ERROR - Failed to start sendMessage, return code %d\n", rc);
#ifdef DEBUG_PRINT_MQTT
    if(logfile && logfile!=stderr) {
      fprintf(logfile,"PAHO_ERROR - Failed to start sendMessage, return code %d\n", rc);
    }
#endif
  }
}

void PahoWrapper::writeState(int circuit, const char *msg) {
  sendToPaho(topicState[circuit], msg);
}

void PahoWrapper::markAvailable(bool avail) {
  int i;
  for(i=0;i<8;i++) {
    if(topicAvailability[i]) {
      sendToPaho(topicAvailability[i], avail?"Online":"Offline");
    }
  }
}

void PahoWrapper::mqttSetup() {
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  int rc;
  MQTTAsync_create(&pahoClient, mqttServer, "Curb2MQTT/1.0", MQTTCLIENT_PERSISTENCE_NONE, NULL);
  MQTTAsync_setCallbacks(pahoClient, NULL, _pahoOnConnLost, NULL, NULL);
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = _pahoOnConnect;
  conn_opts.onFailure = _pahoOnConnectFailure;
  conn_opts.context = (void*)this;
  if ((rc = MQTTAsync_connect(pahoClient, &conn_opts)) != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "PAHO_ERROR - Failed to start connect, return code %d\n", rc);
#ifdef DEBUG_PRINT_MQTT
    if(logfile && logfile!=stderr) {
      fprintf(logfile, "PAHO_ERROR - Failed to start connect, return code %d\n", rc);
    }
#endif
  }
}

//
// https://eclipse.dev/paho/files/mqttdoc/MQTTAsync/html/publish.html
//
void PahoWrapper::pahoOnConnLost(char *cause) {
  fprintf(stderr,"PAHO_ERROR - Connection Lost - cause %s\n", cause);
#ifdef DEBUG_PRINT_MQTT
    if(logfile && logfile!=stderr) {
      fprintf(logfile, "PAHO_ERROR - Connection Lost - cause %s\n", cause);
    }
#endif
}

void PahoWrapper::pahoOnConnectFailure(MQTTAsync_failureData* response) {
  fprintf(stderr,"PAHO_ERROR - Connect failed, rc %d\n", response ? response->code : 0);
#ifdef DEBUG_PRINT_MQTT
    if(logfile && logfile!=stderr) {
      fprintf(logfile, "PAHO_ERROR - Connect failed, rc %d\n", response ? response->code : 0);
    }
#endif
}

void PahoWrapper::pahoOnSendFailure(MQTTAsync_failureData* response) {
  fprintf(stderr,"PAHO_ERROR - Send failed, rc %d\n", response ? response->code : 0);
#ifdef DEBUG_PRINT_MQTT
    if(logfile && logfile!=stderr) {
      fprintf(logfile, "PAHO_ERROR - Send failed, rc %d\n", response ? response->code : 0);
    }
#endif
}

void PahoWrapper::pahoOnConnect(MQTTAsync_successData* response) {
#ifdef DEBUG_PRINT_MQTT
  if(logfile) {
    fprintf(logfile,"PAHO - onConnect complete\n");
  }
#endif
}

void PahoWrapper::pahoOnSend(MQTTAsync_successData* response)
{
#ifdef DEBUG_PRINT_MQTT
  if(logfile) {
    fprintf(logfile,"PAHO - onSend complete\n");
  }
#endif
}

//
// https://eclipse.dev/paho/files/mqttdoc/MQTTAsync/html/publish.html
//
static void _pahoOnConnLost(void *context, char *cause) {
  ((PahoWrapper*)(context))->pahoOnConnLost(cause);
}

static void _pahoOnConnectFailure(void* context, MQTTAsync_failureData* response) {
  ((PahoWrapper*)(context))->pahoOnConnectFailure(response);
}

static void _pahoOnSendFailure(void* context, MQTTAsync_failureData* response) {
  ((PahoWrapper*)(context))->pahoOnSendFailure(response);
}

static void _pahoOnConnect(void* context, MQTTAsync_successData* response) {
  ((PahoWrapper*)(context))->pahoOnConnect(response);
}

static void _pahoOnSend(void* context, MQTTAsync_successData* response) {
  ((PahoWrapper*)(context))->pahoOnSend(response);
}

