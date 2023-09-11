#ifndef _WEBSOCKETH
#define _WEBSOCKETH

class WebSocket {

private:
  HINTERNET hSession;   // the WinHttp session
  HINTERNET hConnect;   // the WinHttp connection to the server
  HINTERNET hWebsocket; // the WinHttp client websocket

  int ensureClosed();  // close out and clean up (return closure status or -1)
  
public:
  WebSocket();
  int createWebSocket(LPCWSTR agent, LPCWSTR host, LPCWSTR path);
  int looper(void(*UTFhandler)(const char *));   // receive stuff, return close status
  void postUTF8(const char *);                   // post message to websocket
};

#endif
