
INCLUDES=../socket.io-client-cpp/src
OPTS=/MD /EHsc /I $(INCLUDES)

Curb2MQTT.exe: Curb2MQTT.cpp
	cl $(OPTS) Curb2MQTT.cpp /link WinHttp.lib 

clean:
	cmd /c del /q Curb2MQTT.obj Curb2MQTT.exe
