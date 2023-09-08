#ifndef _CONFIGH
#define _CONFIGH

class Config {

private:
  char *curbUsername;
  char *curbPassword;
  char *curbClientId;
  char *curbClientSecret;
  
public:
  Config();
  void readConfig(const char *fname);
  const char *getCurbUsername();
  const char *getCurbPassword();
  const char *getCurbClientId();
  const char *getCurbClientSecret();

};

#endif
