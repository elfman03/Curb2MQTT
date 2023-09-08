#include <windows.h>
#include <winhttp.h>
#include <stdio.h>

//#define DEBUG_PRINT

#define API_HOST L"app.energycurb.com"
#define API_PATH L"/socket.io/?EIO=3&transport=websocket"
#define AUTH_MAX 4096
#define AUTH_HOST L"energycurb.auth0.com"
#define AUTH_PATH  L"/oauth/token"

// Stuff to load from config file
//
char *CURB_USERNAME;
char *CURB_PASSWORD;
char *CURB_CLIENT_ID;
char *CURB_CLIENT_SECRET;

char *auth_buf=0, *auth_code=0;

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

// based on Visual Studio Example
// https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview#using-the-winhttp-api-to-access-the-web

void get_curb_token() {
  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  BOOL  bResults = FALSE;
  HINTERNET  hSession = NULL, 
             hConnect = NULL,
             hRequest = NULL;

  char post_data[1024];
  sprintf(post_data,"{\"grant_type\": \"password\", \"audience\": \"app.energycurb.com/api\", \"username\": \"%s\", \"password\": \"%s\", \"client_id\": \"%s\", \"client_secret\": \"%s\", \"redirect_uri\": \"http://localhost:8000\"}", CURB_USERNAME, CURB_PASSWORD, CURB_CLIENT_ID, CURB_CLIENT_SECRET);
  DWORD post_len=strlen(post_data);
  wchar_t *post_head=L"Content-Type: application/json\r\n";

  if(auth_buf) {
    delete [] auth_buf;
    auth_buf=0;
    auth_code=0;
  }
  auth_buf=new char[AUTH_MAX]; 
  auth_code=0;
  ZeroMemory( auth_buf, AUTH_MAX );

  // Use WinHttpOpen to obtain a session handle.
  hSession = WinHttpOpen( L"Curb2Mqtt/1.0",  
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, 
                          WINHTTP_NO_PROXY_BYPASS, 0 );

  // Specify an HTTP server.

  if( hSession ) {
    hConnect = WinHttpConnect( hSession, AUTH_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0 );
  } else {
    printf( "WinHttpOpen Error %d has occurred.\n", GetLastError( ) );
    return;
  }

  // Create an HTTP request handle.

  if( hConnect ) {
    hRequest = WinHttpOpenRequest( hConnect, L"POST", AUTH_PATH,
                                   NULL, WINHTTP_NO_REFERER, 
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                   WINHTTP_FLAG_SECURE );
  } else {
    printf( "WinHttpConnect Error %d has occurred.\n", GetLastError( ) );
    WinHttpCloseHandle( hSession );
    return;
  }

  // Send a request.
  if( hRequest ) {
    bResults = WinHttpSendRequest( hRequest, post_head, -1, post_data, post_len, post_len, 0 );
  } else {
    printf( "WinHttpOpenRequest Error %d has occurred.\n", GetLastError( ) );
    WinHttpCloseHandle( hSession );
    WinHttpCloseHandle( hConnect );
    return;
  }

  // End the request.
  if( bResults ) {
    bResults = WinHttpReceiveResponse( hRequest, NULL );
  } else {
    printf( "WinHttpSendRequest Error %d has occurred.\n", GetLastError( ) );
    WinHttpCloseHandle( hRequest );
    WinHttpCloseHandle( hSession );
    WinHttpCloseHandle( hConnect );
    return;
  }

  // Keep checking for data until there is nothing left.
  if( bResults ) {
    int index=0;
    DWORD dwStatusCode=0,
          dwSize=sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &dwStatusCode, &dwSize, 
                        WINHTTP_NO_HEADER_INDEX);
#ifdef DEBUG_PRINT
    printf("get_curb_token recv status=%d\n",dwStatusCode);
#endif

    do 
    {
      // Check for available data.
      dwSize = 0;
      if( !WinHttpQueryDataAvailable( hRequest, &dwSize ) )
        printf( "Error %u in WinHttpQueryDataAvailable.\n",
                GetLastError( ) );

      if(index+dwSize+1 > AUTH_MAX) {
        printf("CURB AUTH failed\n");
        exit(0);
      }
      if( !WinHttpReadData( hRequest, (LPVOID)&auth_buf[index], dwSize, &dwDownloaded ) ) {
        printf( "Error %u in WinHttpReadData.\n", GetLastError( ) );
      } else {
#ifdef DEBUG_PRINT
        printf( "%s", &auth_buf[index] );
#endif
      }
      index=index+dwSize;
    } while( dwSize > 0 );
    char *p=strstr(auth_buf,"\"access_token\":\"");
    p=&p[16];
    char *p2=strstr(p,"\"");
    p2[0]=0;
#ifdef DEBUG_PRINT
    printf("\n---\n%s\n---\n",p);
#endif
    auth_code=p;
  } else {
    printf( "WinHttpReceiveResponse Error %d has occurred.\n", GetLastError( ) );
    WinHttpCloseHandle( hRequest );
    WinHttpCloseHandle( hSession );
    WinHttpCloseHandle( hConnect );
    return;
  }

  // Report any errors.
  if( !bResults ) {
    printf( "Error %d has occurred.\n", GetLastError( ) );
  } 

  // Close any open handles.
  if( hRequest ) WinHttpCloseHandle( hRequest );
  if( hConnect ) WinHttpCloseHandle( hConnect );
  if( hSession ) WinHttpCloseHandle( hSession );
}

// based on https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WinhttpWebsocket/cpp/WinhttpWebsocket.cpp

void create_websocket() {
  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  BOOL  bResults = FALSE;
  BYTE rgbBuffer[1024];
  BYTE *pbCurrentBufferPointer = rgbBuffer;
  DWORD dwBufferLength = ARRAYSIZE(rgbBuffer);
  DWORD dwBytesTransferred = 0;
  DWORD dwCloseReasonLength = 0;
  WINHTTP_WEB_SOCKET_BUFFER_TYPE eBufferType;
  HINTERNET  hSession = NULL, 
             hConnect = NULL,
             hRequest = NULL,
             hWebsocket=NULL;
  wchar_t *auth_head=new wchar_t[AUTH_MAX+100];
  swprintf(auth_head,L"authorization: Bearer %hs\r\n",auth_code);
  wprintf(L"auth_head=%s\n",auth_head);

  // Use WinHttpOpen to obtain a session handle.
  hSession = WinHttpOpen( L"Curb2Mqtt/1.0",  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0 );

  // Specify an HTTP server.

  if( hSession ) {
    hConnect = WinHttpConnect( hSession, API_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0 );
  } else {
    printf("WinHttpOpen failure %d\n",GetLastError());
    return;
  }

  // Create an HTTP request handle.
  if( hConnect ) {
    hRequest = WinHttpOpenRequest( hConnect, L"GET", API_PATH,
                                   NULL, WINHTTP_NO_REFERER, 
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                   WINHTTP_FLAG_SECURE );
  } else {
    printf("WinHttpConnect failure %d\n",GetLastError());
    WinHttpCloseHandle(hSession);
    return;
  }

  //
  // Request protocol upgrade from http to websocket.
  //
#pragma prefast(suppress:6387, "WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET does not take any arguments.")
  if( hRequest ) {
    bResults = WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);
  } else {
    printf("WinHttpOpenRequest failure %d\n",GetLastError());
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return;
  }

  // Send a request.
  if( bResults ) {
    bResults = WinHttpSendRequest( hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0 );
    //bResults = WinHttpSendRequest( hRequest, auth_head, -1, NULL, 0, 0, 0 );
  } else {
    printf("WinHttpSetOption failure %d\n",GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return;
  }

  // End the request.
  if( bResults ) {
    bResults = WinHttpReceiveResponse( hRequest, NULL );
  } else {
    printf("WinHttpSendRequest failure %d\n",GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return;
  }

  if( bResults ) {
    // https://stackoverflow.com/questions/23906654/how-to-get-http-status-code-from-winhttp-request
    DWORD dwStatusCode=0,
          dwSize=sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &dwStatusCode, &dwSize, 
                        WINHTTP_NO_HEADER_INDEX);
    printf("initialize websocket recv status=%d\n",dwStatusCode);
    hWebsocket = WinHttpWebSocketCompleteUpgrade(hRequest, NULL);
  } else {
    printf("WinHttpReceiveResponse failure %d\n",GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return;
  }

  if(hWebsocket) {
    WinHttpCloseHandle(hRequest);
    hRequest= NULL;
    printf("Succesfully upgraded to websocket protocol\n");
  } else {
    printf("WinHttpWebSocketCompleteUpgrade failure %d\n",GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return;
  }

  char *msg="40/api/circuit-data";
  //bResults=WinHttpWebSocketSend(hWebsocket,WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,msg,strlen(msg)+1);
  bResults=WinHttpWebSocketSend(hWebsocket,WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,msg,strlen(msg)+1);
  printf("bResults=%d\n",bResults);

  eBufferType=WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
  while(eBufferType==WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE) {
    bResults = WinHttpWebSocketReceive(hWebsocket,
                                       pbCurrentBufferPointer,
                                       dwBufferLength,
                                       &dwBytesTransferred,
                                       &eBufferType);
    if(bResults) { 
      printf("WinHttpWebSocketReceive failure %d\n",GetLastError());
      printf("DANGER WILL ROBINSON websocket recv failure\n");
    } else {
      pbCurrentBufferPointer[dwBytesTransferred]=0;
      printf("got fragment size %d type=%d msg=%s\n",dwBytesTransferred,eBufferType,pbCurrentBufferPointer);
    }
  }
  printf("GOT FULL MESSAGE");

  // Close any open handles.
  if( hRequest ) WinHttpCloseHandle( hRequest );
  if( hConnect ) WinHttpCloseHandle( hConnect );
  if( hSession ) WinHttpCloseHandle( hSession );
}

void main() {
  printf("Loading Config File\n");
  read_config();
  printf("Retrieving Curb Access Token\n");
  get_curb_token();
  printf("Creating WebSocket\n");
  create_websocket();
}

