##
## Copyright (C) 2023, Chris Elford
## 
## SPDX-License-Identifier: MIT
##

OPTS=/MD /EHsc 
OBJS=Config.obj AuthToken.obj WebSocket.obj CircuitStateManager.obj PahoWrapper.obj
PAHO_I=../paho.mqtt.c/src
PAHO_L=../paho.mqtt.c/src/Release/paho-mqtt3a.lib

Curb2MQTT.exe: Curb2MQTT.cpp $(OBJS)
	cl $(OPTS) /I $(PAHO_I) Curb2MQTT.cpp /link $(OBJS) WinHttp.lib $(PAHO_L)

Config.obj: Config.cpp Config.h global.h
	cl $(OPTS) /c Config.cpp

AuthToken.obj: AuthToken.cpp AuthToken.h Config.h global.h
	cl $(OPTS) /c AuthToken.cpp

WebSocket.obj: WebSocket.cpp WebSocket.h global.h
	cl $(OPTS) /c WebSocket.cpp

CircuitStateManager.obj: CircuitStateManager.cpp CircuitStateManager.h Config.h PahoWrapper.h global.h 
	cl $(OPTS) /I $(PAHO_I) /c CircuitStateManager.cpp

PahoWrapper.obj: PahoWrapper.cpp PahoWrapper.h Config.h global.h
	cl $(OPTS) /I $(PAHO_I) /c PahoWrapper.cpp

clean:
	cmd /c del /q Config.obj
	cmd /c del /q AuthToken.obj
	cmd /c del /q WebSocket.obj
	cmd /c del /q CircuitStateManager.obj
	cmd /c del /q PahoWrapper.obj
	cmd /c del /q Curb2MQTT.obj Curb2MQTT.exe

test:
	.\Curb2MQTT.exe
