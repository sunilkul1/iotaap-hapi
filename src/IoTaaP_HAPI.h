/*
 * IoTaaP HAPI - Hardware API Library
 * Connectivity platform for rapid development
 * 
 * IoTaaP Team <contact@iotaap.io>
 * https://www.iotaap.io
 * 
 */

#ifndef __IOTAAP_HAPI_H__
#define __IOTAAP_HAPI_H__

#define HAPI_VERSION "0.1.3"
#define DEVICE_STATUS_PERIOD 500
#define DEVICE_OTA_CHECK_PERIOD 2000

#include <Arduino.h>
#include <IoTaaP.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <time.h>

class IoTaaP_HAPI
{
public:
    IoTaaP_HAPI(String fwVersion);
    int configure(const char *deviceID, const char *deviceToken, const char *mqttServer, const char *mqttUsername, const char *mqttPassword, MQTT_CALLBACK_SIGNATURE, const char *groupID = '\0', const char *groupToken = '\0');
    int devicePublish(const char *payload);
    int publish(const char *payload, const char *topic);
    int subscribe(const char *topic);
    int unsubscribe(const char *topic);
    void apiLoop(bool sendStates = false);
    void callbackInnerFunction(char *topic, byte *message, unsigned int length);

private:
    IoTaaP iotaapCore;
    int connectToCloud(const char *server, const char *user, const char *password, MQTT_CALLBACK_SIGNATURE, const char *clientID = '\0');
    int publishStatus();
    void setClock();
    void checkUpdate();
    void otaUpdate();
    WiFiClientSecure _client;
    HTTPClient _httpClient;
    const char *_deviceID;
    const char *_deviceToken;
    const char *_mqttServer;
    const char *_mqttUsername;
    const char *_mqttPassword;
    const char *_groupID;
    const char *_groupToken;
    unsigned long int _uptime;
    unsigned long int _otaUpdateNow; // Detect passed time between update check
    unsigned long int _otaUpdatePrev; // Detect passed time between update check
    String _fwVersion;
    unsigned long int _sentMessages;
    unsigned long int _receivedMessages;
    unsigned long int _disconnects;
    unsigned long int _uptimePrev;
    unsigned long int _uptimeCur;
};

#endif