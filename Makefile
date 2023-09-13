
INCLUDES=../socket.io-client-cpp/src
OPTS=/MD /EHsc /I $(INCLUDES)
OBJS=Config.obj AuthToken.obj WebSocket.obj CircuitStateManager.obj

Curb2MQTT.exe: Curb2MQTT.cpp $(OBJS)
	cl $(OPTS) Curb2MQTT.cpp /link $(OBJS) WinHttp.lib

Config.obj: Config.cpp Config.h global.h
	cl $(OPTS) /c Config.cpp

AuthToken.obj: AuthToken.cpp AuthToken.h Config.h global.h
	cl $(OPTS) /c AuthToken.cpp

WebSocket.obj: WebSocket.cpp WebSocket.h global.h
	cl $(OPTS) /c WebSocket.cpp

CircuitStateManager.obj: CircuitStateManager.cpp CircuitStateManager.h global.h
	cl $(OPTS) /c CircuitStateManager.cpp

clean:
	cmd /c del /q Config.obj
	cmd /c del /q AuthToken.obj
	cmd /c del /q WebSocket.obj
	cmd /c del /q CircuitStateManager.obj
	cmd /c del /q Curb2MQTT.obj Curb2MQTT.exe

test:
	.\Curb2MQTT.exe
