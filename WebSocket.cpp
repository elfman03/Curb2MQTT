#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "global.h"
#include "WebSocket.h"

//socket.io useful reference:
//https://socket.io/docs/v4/socket-io-protocol/#format

WebSocket::WebSocket() { 
  logfile=0;
  hSession=hConnect=hWebsocket=0;
}

// Based on Windows Samples
//
// https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/WinhttpWebsocket/cpp
// https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview#using-the-winhttp-api-to-access-the-web
// see https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WinhttpWebsocket/cpp/WinhttpWebsocket.cpp
//
int WebSocket::ensureClosed() {
  DWORD ret;
  BYTE closeReason[123];
  USHORT closeStatus=0;
  DWORD crl; // close reason length returned by server

  if(hWebsocket) {
    ret=WinHttpWebSocketClose(hWebsocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
    if(ret!=ERROR_SUCCESS) {
      fprintf(stderr,"WinHttpWebSocketClose failure %d\n",GetLastError());
      if(logfile && logfile!=stderr) { fprintf(logfile,"WinHttpWebSocketClose failure %d\n",GetLastError()); }
    } else {
      ret = WinHttpWebSocketQueryCloseStatus(hWebsocket, &closeStatus, closeReason, ARRAYSIZE(closeReason), &crl);
      if(ret!=ERROR_SUCCESS) {
        fprintf(stderr, "WinHttpQueryCloseStatus failure %d\n",GetLastError());
        if(logfile && logfile!=stderr) { fprintf(logfile, "WinHttpQueryCloseStatus failure %d\n",GetLastError()); }
      } else {
#ifdef DEBUG_PRINT_WEBSOCKET
        if(logfile) {
          wfprintf(logfile,L"The server closed the connection with status code: '%d' and reason: '%.*S'\n", 
                           (int)closeStatus, crl, closeReason);
        }
#endif
      }
    }
    WinHttpCloseHandle(hWebsocket); 
    hWebsocket=0;
  }
  if(hConnect) { WinHttpCloseHandle(hConnect); hConnect=0; }
  if(hSession) { WinHttpCloseHandle(hSession); hSession=0; }
  return closeStatus?closeStatus:-1;
}

//
// Connect to host and get a newly converted websocket
//
int WebSocket::createWebSocket(LPCWSTR agent, LPCWSTR host, LPCWSTR path, FILE *logfile) {
  BOOL  bResults = FALSE;
  HINTERNET  hRequest = NULL;

  //
  // Use WinHttpOpen to obtain a session handle.
  //
  hSession = WinHttpOpen( agent,  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0 );
  if(!hSession) {
    fprintf(stderr,"WinHttpOpen failure %d\n",GetLastError());
    if(logfile && logfile!=stderr) {fprintf(logfile,"WinHttpOpen failure %d\n",GetLastError()); }
  } else {
    //
    // Specify an HTTP server.
    //
    hConnect = WinHttpConnect( hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0 );
    if(!hConnect) {
      fprintf(stderr,"WinHttpConnect failure %d\n",GetLastError());
      if(logfile && logfile!=stderr) { fprintf(logfile,"WinHttpConnect failure %d\n",GetLastError()); }
    } else {
      //
      // Create an HTTP request handle.
      //
      hRequest = WinHttpOpenRequest( hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE );
      if(!hRequest) {
        fprintf(stderr,"WinHttpOpenRequest failure %d\n",GetLastError());
        if(logfile && logfile!=stderr) { fprintf(logfile,"WinHttpOpenRequest failure %d\n",GetLastError()); }
      } else {
        //
        // Request protocol upgrade from http to websocket.
        //
#pragma prefast(suppress:6387, "WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET does not take any arguments.")
        bResults = WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);
        if(!bResults) {
          fprintf(stderr, "WinHttpSetOption failure %d\n",GetLastError());
          if(logfile && logfile!=stderr) { fprintf(logfile, "WinHttpSetOption failure %d\n",GetLastError()); }
        } else {
          //
          // Send a request.
          //
          bResults = WinHttpSendRequest( hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0 );
          if(!bResults) {
            fprintf(stderr, "WinHttpSendRequest failure %d\n",GetLastError());
            if(logfile && logfile!=stderr) { fprintf(logfile, "WinHttpSendRequest failure %d\n",GetLastError()); }
          } else {
            //
            // End the request.
            //
            bResults = WinHttpReceiveResponse( hRequest, NULL );
            if(!bResults) {
              fprintf(stderr, "WinHttpReceiveResponse failure %d\n",GetLastError());
              if(logfile && logfile!=stderr) { fprintf(logfile, "WinHttpReceiveResponse failure %d\n",GetLastError()); }
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
                fprintf(stderr, "WinHttpQueryHeaders response code not 101: %d\n",GetLastError());
                if(logfile && logfile!=stderr) { fprintf(logfile, "WinHttpQueryHeaders response code not 101: %d\n",GetLastError()); }
              } else {
                //
                // Complete client side Websocket promotion
                //
                hWebsocket = WinHttpWebSocketCompleteUpgrade(hRequest, NULL);
                if(!hWebsocket) {
                  fprintf(stderr, "WinHttpWebSocketCompleteUpgrade failure %d\n",GetLastError());
                  if(logfile && logfile!=stderr) { fprintf(logfile, "WinHttpWebSocketCompleteUpgrade failure %d\n",GetLastError()); }
                } else {
                  WinHttpCloseHandle(hRequest);
                  hRequest=0;
#ifdef DEBUG_PRINT_WEBSOCKET
                  if(logfile) { fprintf(logfile, "Successfully upgraded to Client WebSocket!\n"); }
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
    return -1;  // not success
  }
  return 0;     // success
}

void WebSocket::postUTF8(const char *outbuf) {
  DWORD ret;
  if(!hWebsocket) {
    fprintf(stderr, "ERROR: WebSocket is not open.  cannot use it to post\n");
    if(logfile && logfile!=stderr) { fprintf(logfile, "ERROR: WebSocket is not open.  cannot use it to post\n"); }
    return;
  }
#ifdef DEBUG_PRINT_WEBSOCKET
  if(logfile) { fprintf(logfile,"SENDING: %s\n",outbuf); }
#endif
  ret=WinHttpWebSocketSend(hWebsocket,WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,(PVOID)outbuf,strlen(outbuf));
  if(ret!=NO_ERROR) {
    fprintf(stderr, "WinHttpWebSocketSend failure %d\n",GetLastError());
    if(logfile && logfile!=stderr) { fprintf(logfile, "WinHttpWebSocketSend failure %d\n",GetLastError()); }
    return;
  }
  return;
}

int WebSocket::looper(void(*UTFhandler)(const char *)) {
  DWORD ret;
  char buf[8192];
  char *bufp=buf;
  DWORD remaining=8192;
  DWORD incoming;
  WINHTTP_WEB_SOCKET_BUFFER_TYPE bType;
  
  if(!hWebsocket) {
    fprintf(stderr, "ERROR: WebSocket is not open.  cannot use it to loop\n");
    if(logfile && logfile!=stderr) { fprintf(logfile, "ERROR: WebSocket is not open.  cannot use it to loop\n"); }
    return -1;
  }
 
  for(;;) { 
    ret = WinHttpWebSocketReceive(hWebsocket, bufp, remaining, &incoming, &bType);
    buf[incoming]=0;
    if(ret!=NO_ERROR) {
      fprintf(stderr, "WinHttpWebSocketReceive failure ret=%d - lasterr=%d\n",ret, GetLastError());
      if(logfile && logfile!=stderr) { fprintf(logfile, "WinHttpWebSocketReceive failure ret=%d (timeout=12002) - lasterr=%d\n",ret, GetLastError()); }
      return ret;
    } else {
      if(bType==WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
#ifdef DEBUG_PRINT_WEBSOCKET
        //if(logfile) { fprintf(logfile, "WEBSOCKET: Received UTF8 Message -- %d bytes\n",incoming); }
#endif
        UTFhandler(buf);
      } else if(bType==WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE) {
#ifdef DEBUG_PRINT_WEBSOCKET
        if(logfile) { fprintf(logfile, "WEBSOCKET: Received UTF8 Fragment -- %d bytes\n",incoming); }
#endif
      } else if(bType==WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) {
#ifdef DEBUG_PRINT_WEBSOCKET
        if(logfile) { fprintf(logfile, "WEBSOCKET: Received Binary Message -- %d bytes\n",incoming); }
#endif
      } else if(bType==WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE) {
#ifdef DEBUG_PRINT_WEBSOCKET
        if(logfile) { fprintf(logfile, "WEBSOCKET: Received Binary Fragment -- %d bytest\n",incoming); }
#endif
      } else if(bType==WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
#ifdef DEBUG_PRINT_WEBSOCKET
        f(logfile) { fprintf(logfile, "WEBSOCKET: Received %d (Close=4) Buffer -- %d bytes\n",bType, incoming); }
#endif
        return ensureClosed();
      }
    }
  }
  return -1; // not reached
}

