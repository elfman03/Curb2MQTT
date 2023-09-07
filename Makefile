
INCLUDES=../socket.io-client-cpp/src
OPTS=/MD /EHsc /I $(INCLUDES)
#SIOLIB=../socket.io-client-cpp/Release/sioclient.lib
#	cl $(OPTS) curb2mqtt.cpp /link WinHttp.lib Websocket.lib

Curb2MQTT.exe: Curb2MQTT.cpp
	cl $(OPTS) Curb2MQTT.cpp /link WinHttp.lib 

clean:
	cmd /c del /q Curb2MQTT.obj Curb2MQTT.exe
