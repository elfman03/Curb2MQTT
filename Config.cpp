#include <windows.h>
#include <stdio.h>
#include "Curb2MQTT.h"

void read_config() {
  FILE *f;
  int i, ch;
  char buf[4096];
  char *p,*q;

  f=fopen("Curb2MQTT.config","r");
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
  CURB_USERNAME=strdup(&p[14]);
  *q='\n';

  p=strstr(buf,"CURB_PASSWORD=");
  for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
  *q=0;
  CURB_PASSWORD=strdup(&p[14]);
  *q='\n';

  p=strstr(buf,"CURB_CLIENT_ID=");
  for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
  *q=0;
  CURB_CLIENT_ID=strdup(&p[15]);
  *q='\n';

  p=strstr(buf,"CURB_CLIENT_SECRET=");
  for(q=p;(*q) && (*q!='\r') && (*q!='\n');) { q=q+1; }  // find end of config parameter
  *q=0;
  CURB_CLIENT_SECRET=strdup(&p[19]);
  *q='\n';

#ifdef DEBUG_PRINT
  printf("CURB_USERNAME=%s\n",CURB_USERNAME);
  printf("CURB_PASSWORD=%s\n",CURB_PASSWORD);
  printf("CURB_CLIENT_ID=%s\n",CURB_CLIENT_ID);
  printf("CURB_CLIENT_SECRET=%s\n",CURB_CLIENT_SECRET);
#endif
}

