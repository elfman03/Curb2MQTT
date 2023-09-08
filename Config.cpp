#include <windows.h>
#include <stdio.h>
#include "Config.h"

const char *Config::getCurbUsername()     { return curbUsername;     }
const char *Config::getCurbPassword()     { return curbPassword;     }
const char *Config::getCurbClientId()     { return curbClientId;     }
const char *Config::getCurbClientSecret() { return curbClientSecret; }

Config::Config() { 
  curbUsername=0;
  curbPassword=0;
  curbClientId=0;
  curbClientSecret=0;
}

void Config::readConfig(const char *fname) {
  FILE *f;
  int i, ch;
  char buf[4096];
  char *p,*q;

  if(curbUsername)     { free(curbUsername);     curbUsername=0;     }
  if(curbPassword)     { free(curbPassword);     curbPassword=0;     }
  if(curbClientId)     { free(curbClientId);     curbClientId=0;     }
  if(curbClientSecret) { free(curbClientSecret); curbClientSecret=0; }

  f=fopen(fname,"r");
  if(!f) {
    printf("Could not open Curb2MQTT.config\n");
    exit(1);
  }

  for(i=0;i<4095;i++) {
     ch=fgetc(f);
     if(ch==EOF) {
        buf[i]=0;
        i=4095;
     } else {
        buf[i]=(char)ch;
     }
  }
  if(ch!=EOF) {
    printf("Excessively long Curb2MQTT.config\n");
    exit(1);
  }
  //printf("buf=%s\n",buf);

  p=strstr(buf,"CURB_USERNAME=");
  for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
  *q=0;
  curbUsername=strdup(&p[14]);
  *q='\n';

  p=strstr(buf,"CURB_PASSWORD=");
  for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
  *q=0;
  curbPassword=strdup(&p[14]);
  *q='\n';

  p=strstr(buf,"CURB_CLIENT_ID=");
  for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
  *q=0;
  curbClientId=strdup(&p[15]);
  *q='\n';

  p=strstr(buf,"CURB_CLIENT_SECRET=");
  for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
  *q=0;
  curbClientSecret=strdup(&p[19]);
  *q='\n';

#ifdef DEBUG_PRINT
  printf("CURB_USERNAME=%s\n",curbUsername);
  printf("CURB_PASSWORD=%s\n",curbPassword);
  printf("CURB_CLIENT_ID=%s\n",curbClientId);
  printf("CURB_CLIENT_SECRET=%s\n",curbClientSecret);
#endif
}

