
INCLUDES=../socket.io-client-cpp/src
OPTS=/MD /EHsc /I $(INCLUDES)
OBJS=Config.obj AuthToken.obj WebSocketInit.obj

Curb2MQTT.exe: Curb2MQTT.cpp Curb2MQTT.h $(OBJS)
	cl $(OPTS) Curb2MQTT.cpp /link $(OBJS) WinHttp.lib

Config.obj: Config.cpp Curb2MQTT.h
	cl $(OPTS) /c Config.cpp

AuthToken.obj: AuthToken.cpp Curb2MQTT.h
	cl $(OPTS) /c AuthToken.cpp

WebSocketInit.obj: WebSocketInit.cpp Curb2MQTT.h
	cl $(OPTS) /c WebSocketInit.cpp

clean:
	cmd /c del /q Config.obj
	cmd /c del /q AuthToken.obj
	cmd /c del /q Curb2MQTT.obj Curb2MQTT.exe

test:
	.\Curb2MQTT.exe
