#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "global.h"
#include "WebSocket.h"

/*
 *
 * API Constants from the Curb API
 *           https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md
 *
 */
#define API_HOST L"app.energycurb.com"
#define API_PATH L"/socket.io/?EIO=3&transport=websocket"

WebSocket::WebSocket() { }

// Based on Windows Samples
//
// https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/WinhttpWebsocket/cpp
// https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview#using-the-winhttp-api-to-access-the-web
// see https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WinhttpWebsocket/cpp/WinhttpWebsocket.cpp
//
void WebSocket::ensureClosed() {
  DWORD ret;
  BYTE closeReason[123];
  USHORT closeStatus;
  DWORD crl; // close reason length returned by server

  if(hWebsocket) {
    ret=WinHttpWebSocketClose(hWebsocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
    if(ret!=ERROR_SUCCESS) {
      printf("WinHttpWebSocketClose failure %d\n",GetLastError());
    } else {
      ret = WinHttpWebSocketQueryCloseStatus(hWebsocket, &closeStatus, closeReason, ARRAYSIZE(closeReason), &crl);
      if(ret!=ERROR_SUCCESS) {
        printf("WinHttpQueryCloseStatus failure %d\n",GetLastError());
      } else {
#ifdef DEBUG_PRINT
        wprintf(L"The server closed the connection with status code: '%d' and reason: '%.*S'\n", (int)closeStatus, crl, closeReason);
#endif
      }
    }
    WinHttpCloseHandle(hWebsocket); 
    hWebsocket=0;
  }
  if(hConnect) { WinHttpCloseHandle(hConnect); hConnect=0; }
  if(hSession) { WinHttpCloseHandle(hSession); hSession=0; }
}

//
// Connect curb and get a newly converted websocket
//
void WebSocket::createWebSocket() {
  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  BOOL  bResults = FALSE;
  BYTE rgbBuffer[1024];
  BYTE *pbCurrentBufferPointer = rgbBuffer;
  DWORD dwBufferLength = ARRAYSIZE(rgbBuffer);
  DWORD dwBytesTransferred = 0;
  DWORD dwCloseReasonLength = 0;
  WINHTTP_WEB_SOCKET_BUFFER_TYPE eBufferType;
  HINTERNET  hRequest = NULL;

//  wchar_t *auth_head=new wchar_t[strlen(token)+100];
//  swprintf(auth_head,L"authorization: Bearer %hs\r\n",token);
//  wprintf(L"auth_head=%s\n",auth_head);

  //
  // Use WinHttpOpen to obtain a session handle.
  //
  hSession = WinHttpOpen( L"Curb2Mqtt/1.0",  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0 );
  if(!hSession) {
    printf("WinHttpOpen failure %d\n",GetLastError());
  } else {
    //
    // Specify an HTTP server.
    //
    hConnect = WinHttpConnect( hSession, API_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0 );
    if(!hConnect) {
      printf("WinHttpConnect failure %d\n",GetLastError());
    } else {
      //
      // Create an HTTP request handle.
      //
      hRequest = WinHttpOpenRequest( hConnect, L"GET", API_PATH,
                                     NULL, WINHTTP_NO_REFERER, 
                                     WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                     WINHTTP_FLAG_SECURE );
      if(!hRequest) {
        printf("WinHttpOpenRequest failure %d\n",GetLastError());
      } else {
        //
        // Request protocol upgrade from http to websocket.
        //
#pragma prefast(suppress:6387, "WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET does not take any arguments.")
        bResults = WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);
        if(!bResults) {
          printf("WinHttpSetOption failure %d\n",GetLastError());
        } else {
          //
          // Send a request.
          //
          bResults = WinHttpSendRequest( hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0 );
          if(!bResults) {
            printf("WinHttpSendRequest failure %d\n",GetLastError());
          } else {
            //
            // End the request.
            //
            bResults = WinHttpReceiveResponse( hRequest, NULL );
            if(!bResults) {
              printf("WinHttpReceiveResponse failure %d\n",GetLastError());
            } else {
              // https://stackoverflow.com/questions/23906654/how-to-get-http-status-code-from-winhttp-request
              DWORD dwStatusCode=0,
                    dwSize=sizeof(dwStatusCode);
              WinHttpQueryHeaders(hRequest,
                                  WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                                  WINHTTP_HEADER_NAME_BY_INDEX,
                                  &dwStatusCode, &dwSize, 
                                  WINHTTP_NO_HEADER_INDEX);
              if(dwStatusCode!=101) {
                printf("WinHttpQueryHeaders response code not 101: %d\n",GetLastError());
              } else {
                //
                // Complete client side Websocket promotion
                //
                hWebsocket = WinHttpWebSocketCompleteUpgrade(hRequest, NULL);
                if(!hWebsocket) {
                  printf("WinHttpWebSocketCompleteUpgrade failure %d\n",GetLastError());
                } else {
                  WinHttpCloseHandle(hRequest);
                  hRequest=0;
#ifdef DEBUG_PRINT
                  printf("Successfully upgraded to Client WebSocket!\n");
#endif
                }
              }
            }
          }
        }
      }
    }
  }
  // 
  // If we were unsuccessful close out all the bad handles
  //
  if(!hWebsocket) {
    if(hRequest) { WinHttpCloseHandle(hRequest); hRequest=0; }
    if(hConnect) { WinHttpCloseHandle(hConnect); hConnect=0; }
    if(hSession) { WinHttpCloseHandle(hSession); hSession=0; }
  }
  ensureClosed();
}

/*

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
*/

