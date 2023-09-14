#ifndef _PAHOWRAPPER
#define _PAHOWRAPPER

class Config;

class PahoWrapper {

private:
  FILE *logfile;

public:
  PahoWrapper(Config *config);
  void writeToMQTT(const char *topic,const char *msg);
};

#endif
