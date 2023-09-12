#include <windows.h>
#include <stdio.h>
#include "global.h"
#include "Config.h"

FILE       *Config::getLogfile()               { return logfile;             }
const char *Config::getCurbUsername()          { return curbUsername;        }
const char *Config::getCurbPassword()          { return curbPassword;        }
const char *Config::getCurbClientId()          { return curbClientId;        }
const char *Config::getCurbClientSecret()      { return curbClientSecret;    }
const char *Config::getCurbUID()               { return curbUID;             }
const char *Config::getCircuitName(int i)      { return circuitName[i];      }
int         Config::getCircuitThreshold(int i) { return circuitThreshold[i]; }

Config::Config() { 
  logfile=0;
  logfileName=0;
  curbUsername=0;
  curbPassword=0;
  curbClientId=0;
  curbClientSecret=0;
  curbUID=0;
  for(int i=0;i<8;i++) { circuitName[i]=0; circuitThreshold[i]=0; }
}

//
// Read Curb2MQTT.config and populate settings
//
void Config::readConfig(const char *fname) {
  FILE *f;
  int i, ch;
  char buf[4096];
  char *p,*q;

  if(logfile && logfile!=stderr && logfile!=stdout) { fclose(logfile); }
  logfile=0;
  if(logfileName)      { free(logfileName);      logfileName=0;      }
  if(curbUsername)     { free(curbUsername);     curbUsername=0;     }
  if(curbPassword)     { free(curbPassword);     curbPassword=0;     }
  if(curbClientId)     { free(curbClientId);     curbClientId=0;     }
  if(curbClientSecret) { free(curbClientSecret); curbClientSecret=0; }
  if(curbUID)          { free(curbUID);          curbUID=0; }
  for(i=0;i<8;i++) {
    if(circuitName[i]) { free(circuitName[i]);   circuitName[i]=0; }
    circuitThreshold[i]=0;
  }

  f=fopen(fname,"r");
  if(!f) {
    fprintf(stderr,"Could not open Curb2MQTT.config\n");
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
    fprintf(stderr,"Excessively long Curb2MQTT.config\n");
    exit(1);
  }

  p=strstr(buf,"LOGFILE=");
  if(p) {
    for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
    *q=0;
    logfileName=strdup(&p[8]);
    *q='\n';
    if(!strcmp(logfileName,"stderr")) {
      logfile=stderr;
    } else if(!strcmp(logfileName,"stdout")) {
      logfile=stdout;
    } else {
      logfile=fopen(logfileName,"w");
    }
  }

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

  char chartmp[24];
  for(i=0;i<8;i++) {
    sprintf(chartmp,"CIRCUIT_NAME_%d=",i);
    p=strstr(buf,chartmp);
    if(p) {
      for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
      *q=0;
      circuitName[i]=strdup(&p[15]);
      *q='\n';
    }
    sprintf(chartmp,"CIRCUIT_THRESHOLD_%d=",i);
    p=strstr(buf,chartmp);
    if(p) {
      for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
      *q=0;
      circuitThreshold[i]=atoi(&p[20]);
      *q='\n';
    }
  }

#ifdef DEBUG_PRINT_CONFIG
  if(logfile) { 
    fprintf(logfile,"LOGFILE=%s\n",logfileName);
    fprintf(logfile,"CURB_USERNAME=%s\n",curbUsername);
    fprintf(logfile,"CURB_PASSWORD=%s\n",curbPassword);
    fprintf(logfile,"CURB_CLIENT_ID=%s\n",curbClientId);
    fprintf(logfile,"CURB_CLIENT_SECRET=%s\n",curbClientSecret);
    fprintf(logfile,"CURB_UID=%s\n",curbUID);
    for(i=0;i<8;i++) {
      fprintf(logfile,"CIRCUIT_NAME_%d=%s\n",i,circuitName[i]);
      fprintf(logfile,"CIRCUIT_THRESHOLD_%d=%d\n",i,circuitThreshold[i]);
    }
  }
#endif
}

