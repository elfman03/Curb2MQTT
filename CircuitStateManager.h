#ifndef _CIRCUITSTATEMANAGER
#define _CIRCUITSTATEMANAGER

class Config;

class CircuitStateManager {

private:
  FILE *logfile;
  const char *circuitName[8];
  char       *circuitLabel[8];
  int         circuitThreshold[8];
  int         circuitState[8];
  int         circuitStateLast[8];

public:
  CircuitStateManager(Config *config);
  void processDataPacket(const char *payload);
};

#endif
