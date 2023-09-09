#ifndef _WEBSOCKETH
#define _WEBSOCKETH

class AuthToken;

class WebSocket {

private:
  char *authBuf;  // Auth buffer holding auth code from server
  char *authCode; // Subset of AUTH_BUF with the actual auth code
  
public:
  WebSocket();
  void createWebSocket(const char *token);
};

#endif
