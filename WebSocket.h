#ifndef _WEBSOCKETH
#define _WEBSOCKETH

class WebSocket {

private:
  HINTERNET hSession;   // the WinHttp session
  HINTERNET hConnect;   // the WinHttp connection to the server
  HINTERNET hWebsocket; // the WinHttp client websocket

  void ensureClosed();  // close out and clean up
  
public:
  WebSocket();
  int createWebSocket(LPCWSTR agent, LPCWSTR host, LPCWSTR path);
  void looper(void(*UTFhandler)(const char *));  // receive stuff!
  void postUTF8(const char *);                   // post message to websocket
};

#endif
