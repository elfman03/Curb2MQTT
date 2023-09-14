#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <MQTTClient.h>
#include "global.h"
#include "Config.h"
#include "PahoWrapper.h"


PahoWrapper::PahoWrapper(Config *config) {
}

void PahoWrapper::writeToMQTT(const char *topic, const char *msg) {
}
