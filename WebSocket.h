#ifndef _WEBSOCKETH
#define _WEBSOCKETH

class AuthToken;

class WebSocket {

private:
  char *authBuf;        // Auth buffer holding auth code from server
  char *authCode;       // Subset of AUTH_BUF with the actual auth code
  HINTERNET hSession;   // the WinHttp session
  HINTERNET hConnect;   // the WinHttp connection to the server
  HINTERNET hWebsocket; // the WinHttp client websocket

  void ensureClosed();  // close out and clean up
  
public:
  WebSocket();
  void createWebSocket();
};

#endif
