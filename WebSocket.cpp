#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "global.h"
#include "WebSocket.h"

/*
 *
 * API Constants from the Curb API
 *
 */
#define API_HOST L"app.energycurb.com"
#define API_PATH L"/socket.io/?EIO=3&transport=websocket"

WebSocket::WebSocket() { }

/*
 * NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 * NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 * NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 *
 * This code is sloppy and is based on a WinHttp sample.  
 * It realy should be rewritten a bit more tersely.
 *
 */

// Based on Windows Samples
//
// https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/WinhttpWebsocket/cpp
// https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview#using-the-winhttp-api-to-access-the-web
//
// based on Curb API 
// https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md

//
// Connect curb and get a newly converted websocket
//
void WebSocket::createWebSocket(const char *token) {
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
  wchar_t *auth_head=new wchar_t[strlen(token)+100];
  swprintf(auth_head,L"authorization: Bearer %hs\r\n",token);
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

