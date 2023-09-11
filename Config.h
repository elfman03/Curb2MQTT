#ifndef _CONFIGH
#define _CONFIGH

class Config {

private:
  char *curbUsername;
  char *curbPassword;
  char *curbClientId;
  char *curbClientSecret;
  char *curbUID;
  
public:
  Config();
  void readConfig(const char *fname);
  const char *getCurbUsername();
  const char *getCurbPassword();
  const char *getCurbClientId();
  const char *getCurbClientSecret();
  const char *getCurbUID();

};

#endif
