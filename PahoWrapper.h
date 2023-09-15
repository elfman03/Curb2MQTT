#ifndef _PAHOWRAPPER
#define _PAHOWRAPPER

class Config;

class PahoWrapper {

private:
  FILE *logfile;
  const char *mqttServer;
  char *topicState[8];
  char *topicAvailability[8];

public:
  PahoWrapper(Config *config);
  void writeToMQTT(const char *topic,const char *msg);
};

#endif
