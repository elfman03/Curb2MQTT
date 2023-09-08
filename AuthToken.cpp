#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "Curb2MQTT.h"

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

  if(AUTH_BUF) {
    delete [] AUTH_BUF;
    AUTH_BUF=0;
    AUTH_CODE=0;
  }
  AUTH_BUF=new char[AUTH_MAX]; 
  AUTH_CODE=0;
  ZeroMemory( AUTH_BUF, AUTH_MAX );

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
      if( !WinHttpReadData( hRequest, (LPVOID)&AUTH_BUF[index], dwSize, &dwDownloaded ) ) {
        printf( "Error %u in WinHttpReadData.\n", GetLastError( ) );
      } else {
#ifdef DEBUG_PRINT
        printf( "%s", &AUTH_BUF[index] );
#endif
      }
      index=index+dwSize;
    } while( dwSize > 0 );
    char *p=strstr(AUTH_BUF,"\"access_token\":\"");
    p=&p[16];
    char *p2=strstr(p,"\"");
    p2[0]=0;
#ifdef DEBUG_PRINT
    printf("\n---\n%s\n---\n",p);
#endif
    AUTH_CODE=p;
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

