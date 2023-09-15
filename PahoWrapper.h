#ifndef _PAHOWRAPPER
#define _PAHOWRAPPER

class Config;

class PahoWrapper {

private:
  FILE *logfile;
  const char *mqttServer;
  char *topicState[8];
  char *topicAvailability[8];
  void writeMessage(const char *topic, const char *msg);

public:
  PahoWrapper(Config *config);
  void markAvailable(bool avail);
  void writeToMQTT(int circuit, const char *msg);
};

#endif
