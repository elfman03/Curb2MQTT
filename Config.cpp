#include <windows.h>
#include <stdio.h>
#include "global.h"
#include "Config.h"

const char *Config::getCurbUsername()     { return curbUsername;     }
const char *Config::getCurbPassword()     { return curbPassword;     }
const char *Config::getCurbClientId()     { return curbClientId;     }
const char *Config::getCurbClientSecret() { return curbClientSecret; }
const char *Config::getCurbUID()          { return curbUID;          }

Config::Config() { 
  curbUsername=0;
  curbPassword=0;
  curbClientId=0;
  curbClientSecret=0;
  curbUID=0;
}

//
// Read Curb2MQTT.config and populate settings
//
void Config::readConfig(const char *fname) {
  FILE *f;
  int i, ch;
  char buf[4096];
  char *p,*q;

  if(curbUsername)     { free(curbUsername);     curbUsername=0;     }
  if(curbPassword)     { free(curbPassword);     curbPassword=0;     }
  if(curbClientId)     { free(curbClientId);     curbClientId=0;     }
  if(curbClientSecret) { free(curbClientSecret); curbClientSecret=0; }
  if(curbUID)          { free(curbUID);          curbUID=0; }

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

  p=strstr(buf,"CURB_UID=");
  for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
  *q=0;
  curbUID=strdup(&p[9]);
  *q='\n';

#ifdef DEBUG_PRINT_CONFIG
  printf("CURB_USERNAME=%s\n",curbUsername);
  printf("CURB_PASSWORD=%s\n",curbPassword);
  printf("CURB_CLIENT_ID=%s\n",curbClientId);
  printf("CURB_CLIENT_SECRET=%s\n",curbClientSecret);
  printf("CURB_UID=%s\n",curbUID);
#endif
}

