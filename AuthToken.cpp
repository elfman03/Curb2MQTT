/*
 * Copyright (C) 2023, Chris Elford
 * 
 * SPDX-License-Identifier: MIT
 *
 */

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "global.h"
#include "Config.h"
#include "AuthToken.h"

#define AUTH_MAX 4096
#define AUTH_HOST L"energycurb.auth0.com"
#define AUTH_PATH  L"/oauth/token"

AuthToken::AuthToken() { 
  authBuf=0;
  authCode=0;
}

const char *AuthToken::getAuthToken(Config *config, bool forceNew) {
  // If already have an authcode and we dont want to force a new one then just return the current one.
  //
  FILE *log=config->getLogfile();
  if(!forceNew && authCode) { 
#ifdef DEBUG_PRINT_AUTH
     if(log) { fprintf(log,"Returning existing AuthCode\n"); }
#endif
  } else {
    //
    // Otherwise lets get a fresh token
    //
    fetchNewToken(config);
  }
  return authCode;
}


/*
 * NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 * NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 * NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 *
 * This code is sloppy and is based on a WinHttp sample.  
 * It realy should be rewritten a bit more tersely.
 *
 */

// based on Visual Studio Example
// https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview#using-the-winhttp-api-to-access-the-web
// based on Curb API 
// https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md

//
// Connect to curb authentication server and get an authentication Token
// See https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md
//
const char *AuthToken::fetchNewToken(Config *config) {
  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  BOOL  bResults = FALSE;
  HINTERNET  hSession = NULL, 
             hConnect = NULL,
             hRequest = NULL;
  FILE *log=config->getLogfile();

  char post_data[1024];
  sprintf(post_data,"{\"grant_type\": \"password\", \"audience\": \"app.energycurb.com/api\", \"username\": \"%s\", \"password\": \"%s\", \"client_id\": \"%s\", \"client_secret\": \"%s\", \"redirect_uri\": \"http://localhost:8000\"}", config->getCurbUsername(), config->getCurbPassword(), config->getCurbClientId(), config->getCurbClientSecret());
  DWORD post_len=strlen(post_data);
  wchar_t *post_head=L"Content-Type: application/json\r\n";
#ifdef DEBUG_PRINT_AUTH
  if(log) { fprintf(log, "Post payload is %s\n",post_data); }
#endif

  if(authBuf) {
    free(authBuf);
    authBuf=0;
    authCode=0;
  }
  authBuf=(char*)malloc(AUTH_MAX); 
  authCode=0;
  ZeroMemory( authBuf, AUTH_MAX );

  // Use WinHttpOpen to obtain a session handle.
  hSession = WinHttpOpen( L"Curb2Mqtt/1.0",  
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, 
                          WINHTTP_NO_PROXY_BYPASS, 0 );

  // Specify an HTTP server.

  if( hSession ) {
    hConnect = WinHttpConnect( hSession, AUTH_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0 );
  } else {
    fprintf(stderr, "WinHttpOpen Error %d has occurred.\n", GetLastError( ) );
    if(log && log!=stderr) { fprintf(log, "WinHttpOpen Error %d has occurred.\n", GetLastError( ) ); }
    return 0;
  }

  // Create an HTTP request handle.

  if( hConnect ) {
    hRequest = WinHttpOpenRequest( hConnect, L"POST", AUTH_PATH,
                                   NULL, WINHTTP_NO_REFERER, 
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                   WINHTTP_FLAG_SECURE );
  } else {
    fprintf(stderr,"WinHttpConnect Error %d has occurred.\n", GetLastError( ) );
    if(log && log!=stderr) { fprintf(log,"WinHttpConnect Error %d has occurred.\n", GetLastError( ) ); }
    WinHttpCloseHandle( hSession );
    return 0;
  }

  // Send a request.
  if( hRequest ) {
    bResults = WinHttpSendRequest( hRequest, post_head, -1, post_data, post_len, post_len, 0 );
  } else {
    fprintf(stderr,"WinHttpOpenRequest Error %d has occurred.\n", GetLastError( ) );
    if(log && log!=stderr) { fprintf(log,"WinHttpOpenRequest Error %d has occurred.\n", GetLastError( ) ); }
    WinHttpCloseHandle( hSession );
    WinHttpCloseHandle( hConnect );
    return 0;
  }

  // End the request.
  if( bResults ) {
    bResults = WinHttpReceiveResponse( hRequest, NULL );
  } else {
    fprintf(stderr,"WinHttpSendRequest Error %d has occurred.\n", GetLastError( ) );
    if(log && log!=stderr) { fprintf(log,"WinHttpSendRequest Error %d has occurred.\n", GetLastError( ) ); }
    WinHttpCloseHandle( hRequest );
    WinHttpCloseHandle( hSession );
    WinHttpCloseHandle( hConnect );
    return 0;
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
#ifdef DEBUG_PRINT_AUTH
    if(log) { fprintf(log, "get_curb_token recv status=%d\n",dwStatusCode); }
#endif

    do 
    {
      // Check for available data.
      dwSize = 0;
      if( !WinHttpQueryDataAvailable( hRequest, &dwSize ) )
        fprintf(stderr,"Error %u in WinHttpQueryDataAvailable.\n", GetLastError( ) );
        if(log && log!=stderr) { fprintf(log,"Error %u in WinHttpQueryDataAvailable.\n", GetLastError( ) ); }

      if(index+dwSize+1 > AUTH_MAX) {
        fprintf(stderr,"CURB AUTH failed\n");
        if(log && log!=stderr) { fprintf(log,"CURB AUTH failed\n"); }
        exit(0);
      }
      if( !WinHttpReadData( hRequest, (LPVOID)&authBuf[index], dwSize, &dwDownloaded ) ) {
        fprintf(stderr, "Error %u in WinHttpReadData.\n", GetLastError( ) );
        if(log && log!=stderr) { fprintf(log, "Error %u in WinHttpReadData.\n", GetLastError( ) ); }
      } else {
#ifdef DEBUG_PRINT_AUTH
        if(log) { fprintf(log, "%s", &authBuf[index] ); }
#endif
      }
      index=index+dwSize;
    } while( dwSize > 0 );
    char *p=strstr(authBuf,"\"access_token\":\"");
    p=&p[16];
    char *p2=strstr(p,"\"");
    p2[0]=0;
    authCode=p;
#ifdef DEBUG_PRINT_AUTH
    if(log) { fprintf(log, "\n---\n%s\n---\n",authCode); }
#endif
  } else {
    fprintf(stderr, "WinHttpReceiveResponse Error %d has occurred.\n", GetLastError( ) );
    if(log && log!=stderr) { fprintf(log, "WinHttpReceiveResponse Error %d has occurred.\n", GetLastError( ) ); }
    WinHttpCloseHandle( hRequest );
    WinHttpCloseHandle( hSession );
    WinHttpCloseHandle( hConnect );
    return 0;
  }

  // Report any errors.
  if( !bResults ) {
    fprintf(stderr,"Error %d has occurred.\n", GetLastError( ) );
    if(log && log!=stderr) { fprintf(log,"Error %d has occurred.\n", GetLastError( ) ); }
  } 

  // Close any open handles.
  if( hRequest ) WinHttpCloseHandle( hRequest );
  if( hConnect ) WinHttpCloseHandle( hConnect );
  if( hSession ) WinHttpCloseHandle( hSession );
  return authCode;
}

