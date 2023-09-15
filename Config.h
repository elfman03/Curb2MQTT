#ifndef _CONFIGH
#define _CONFIGH

class Config {

private:
  FILE *logfile;
  char *logfileName;
  char *mqttServer;
  char *mqttTopicBase;
  char *curbUsername;
  char *curbPassword;
  char *curbClientId;
  char *curbClientSecret;
  char *curbUID;
  char *circuitName[8];
  int circuitThreshold[8];
  
public:
  Config();
  void readConfig(const char *fname);
  FILE *getLogfile();
  const char *getMqttServer();
  const char *getMqttTopicBase();
  const char *getCurbUsername();
  const char *getCurbPassword();
  const char *getCurbClientId();
  const char *getCurbClientSecret();
  const char *getCurbUID();
  const char *getCircuitName(int i);
  int getCircuitThreshold(int i);
};

#endif
