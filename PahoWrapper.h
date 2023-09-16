#ifndef _PAHOWRAPPER
#define _PAHOWRAPPER
#include <MQTTAsync.h>

class Config;

class PahoWrapper {

private:
  FILE *logfile;
  MQTTAsync pahoClient;
  const char *mqttServer;
  char *topicState[8];
  char *topicAvailability[8];
  void sendToPaho(const char *topic, const char *msg);
  //
  void mqttSetup();

public:
  PahoWrapper(Config *config);
  void markAvailable(bool avail);
  void writeState(int circuit, const char *msg);

  void pahoOnConnLost(char *cause);
  void pahoOnConnectFailure(MQTTAsync_failureData* response);
  void pahoOnConnect(MQTTAsync_successData* response);
  void pahoOnSend(MQTTAsync_successData* response);
  void pahoOnSendFailure(MQTTAsync_failureData* response);

};

#endif
