# Curb2MQTT
Extract data from Curb power monitoring and selectively post to MQTT
This is experimental integration.

Data goes from Curb monitoring box up to cloud then streams back down to client app via websocket interface which then posts relevant information to MQTT.  No documented local way to fetch the stream without the cloud.

Usage: Curb2MQTT.exe

Config: Create a file called Curb2MQTT.config  (see Curb2MQTT.config.sample)
  * MQTT_SERVER         = IP address of your MQTT server
  * MQTT_TOPIC_BASE     = Base topic
  * CURB_USERNAME       = Your Curb Website Username
  * CURB_PASSWORD       = Your Curb Website Password
  * CURB_CLIENT_ID      = CURB API CREDENTIAL (see dependencies below)
  * CURB_CLIENT_SECRET  = CURB API CREDENTIAL (see dependencies below)
  * CURB_UID            = The UID of your curb from the curb website
  * CIRCUIT_NAME_x      = Name of circuit of interest from curb website
  * CIRCUIT_THRESHOLD_x = watts threshold that corresponds to "on"

Platform: Windows

Dependencies:
  * Curb v2 API 3rd party integration
    * https://github.com/Curb-v2/third-party-app-integration
    * API key and details at https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md
  * Microsoft Visual Studio 
    * I use 2022 Community edition x64 developer prompt 
    * C:\Windows\System32\cmd.exe /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
  * Paho Asynchronous Client library - C edition
    * https://github.com/eclipse/paho.mqtt.c
    * Place PAHO header\lib in ..\paho.mqtt.c
    * Dependency: paho-mqtt3a.dll (place in directory with Curb2MQTT.exe)
    * I built paho.mqtt.c from source but you may be able to use the prebuilts at https://github.com/eclipse/paho.mqtt.c/releases
   
