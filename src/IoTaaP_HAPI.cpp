#include "IoTaaP_HAPI.h"
#include "certificate.h"
#include "hapi/src/ota_certificate.h"

/*
TODO:
1. OTA Update checking
2. OTA Update handler
3. Adding variables in device status payload (JSON)
*/

/**
 * @brief Construct a new IoTaaP_HAPI:: IoTaaP_HAPI object
 * 
 */
IoTaaP_HAPI::IoTaaP_HAPI(String fwVersion)
{
    this->_uptime = 0;
    this->_uptimePrev = 0;
    this->_sentMessages = 0;
    this->_receivedMessages = 0;
    this->_disconnects = 0;
    this->_otaUpdateNow = 0;
    this->_otaUpdatePrev = 0;
    this->_fwVersion = String(fwVersion);
}

/**
 * @brief Configure IoTaaP Cloud connection
 * 
 * @param deviceID - Device ID from IoTaaP Cloud
 * @param deviceToken - Device Token from IoTaaP Cloud
 * @param mqttServer - IoTaaP Cloud MQTT server instance
 * @param mqttUsername - MQTT Username from IoTaaP Cloud
 * @param mqttPassword - MQTT Password from IoTaaP Cloud
 * @param groupID - (optional) Group ID from IoTaaP Cloud
 * @param groupToken - (optional) Group Token from IoTaaP Cloud
 * @return int - Returns 0 if successfull
 */
int IoTaaP_HAPI::configure(const char *deviceID, const char *deviceToken, const char *mqttServer, const char *mqttUsername, const char *mqttPassword, MQTT_CALLBACK_SIGNATURE, const char *groupID, const char *groupToken)
{
    this->_deviceID = deviceID;
    this->_deviceToken = deviceToken;
    this->_mqttServer = mqttServer;
    this->_mqttUsername = mqttUsername;
    this->_mqttPassword = mqttPassword;
    this->_groupID = groupID;
    this->_groupToken = groupToken;

    this->connectToCloud(this->_mqttServer, this->_mqttUsername, this->_mqttPassword, callback, this->_deviceID);

    return 0;
}

/**
 * @brief Execute MQTT (cloud) connection
 * 
 * @param server - MQTT server URL
 * @param user - MQTT Username
 * @param password - MQTT Password
 * @param clientID - Client ID
 * @return int - Returns 0 if successfull
 */
int IoTaaP_HAPI::connectToCloud(const char *server, const char *user, const char *password, MQTT_CALLBACK_SIGNATURE, const char *clientID)
{
    this->iotaapCore.mqtt.connect(clientID, server, 8883, callback, true, user, password, iotaap_mqtts_certificate);

    return 0;
}

DynamicJsonDocument doc(256); // Dynamic JSON doucmnet used for device Status

/**
 * @brief Publishes device status periodically
 * 
 * @return int Returns 0 if successfull
 */
int IoTaaP_HAPI::publishStatus()
{

    // Uptime
    this->_uptime = millis(); // TODO - Implement overflow detection
    //

    doc["battery"] = this->iotaapCore.misc.getBatteryPercentage();
    doc["uptime"] = this->_uptime / 1000;
    doc["api_version"] = HAPI_VERSION;
    doc["fw_version"] = this->_fwVersion;

    char deviceStatus[256];

    serializeJson(doc, deviceStatus);

    String topic = "/" + String(this->_mqttUsername) + "/devices/" + String(this->_deviceID) + "/status";
    char topicChar[256];
    topic.toCharArray(topicChar, sizeof(topicChar));

    if ((this->_uptime - this->_uptimePrev) >= DEVICE_STATUS_PERIOD)
    {
        this->_uptimePrev = this->_uptime;
        this->iotaapCore.mqtt.publish(topicChar, deviceStatus, false);
    }

    return 0;
}

/**
 * @brief Publishes payload to the device topic
 * 
 * @param payload - Payload (recomended: JSON)
 * @return int Returns 0 if successfull
 */
int IoTaaP_HAPI::devicePublish(const char *payload)
{
    String topic = "/" + String(this->_mqttUsername) + "/devices/" + String(this->_deviceID) + String(topic);
    char topicChar[256];
    topic.toCharArray(topicChar, sizeof(topicChar));

    this->iotaapCore.mqtt.publish(topicChar, payload, false);

    return 0;
}

/**
 * @brief Pulishes payload to the topic. Root topic (username) will be automatically added
 * 
 * @param payload - Payload (recomended: JSON)
 * @param uTopic - Topic to publish to
 * @return int - Returns 0 if successfull
 */
int IoTaaP_HAPI::publish(const char *payload, const char *uTopic)
{
    String topic = "/" + String(this->_mqttUsername) + "/" + String(uTopic);
    char topicChar[256];
    topic.toCharArray(topicChar, sizeof(topicChar));

    this->iotaapCore.mqtt.publish(topicChar, payload, false);

    return 0;
}

/**
 * @brief Subsribe to a specific topic. Root topic (username) will be added automatically
 * 
 * @param uTopic - Topic to subscribe to
 * @return int Returns 0 if successfull
 */
int IoTaaP_HAPI::subscribe(const char *uTopic)
{
    String topic = "/" + String(this->_mqttUsername) + String(uTopic);
    char topicChar[256];
    topic.toCharArray(topicChar, sizeof(topicChar));

    this->iotaapCore.mqtt.subscribe(topicChar);

    return 0;
}

/**
 * @brief Unsubscribe from the specific topic
 * 
 * @param uTopic Topic (without root)
 * @return int Returns 0 if successfull
 */
int IoTaaP_HAPI::unsubscribe(const char *uTopic)
{
    String topic = "/" + String(this->_mqttUsername) + String(uTopic);
    char topicChar[256];
    topic.toCharArray(topicChar, sizeof(topicChar));

    this->iotaapCore.mqtt.unsubscribe(topicChar);

    return 0;
}

/**
 * @brief API function that will publish IoTaaP input states to the topic 'api/transfer'. Almost non-blocking.
 * 
 */
void IoTaaP_HAPI::apiLoop(bool sendStates)
{
    this->iotaapCore.mqtt.keepAlive();
    this->publishStatus();

    this->_otaUpdateNow = millis();
    if ((this->_otaUpdateNow - this->_otaUpdatePrev) > DEVICE_OTA_CHECK_PERIOD)
    {
        this->_otaUpdatePrev = this->_otaUpdateNow;
        this->checkUpdate();
    }

    if (sendStates)
    {
        DynamicJsonDocument _doc(1024);

        _doc["device"] = this->_deviceID;

        _doc["data"]["onboard"]["but1"] = this->iotaapCore.misc.getBUT1();
        _doc["data"]["onboard"]["but2"] = this->iotaapCore.misc.getBUT2();

        _doc["data"]["digital"]["2"] = this->iotaapCore.misc.getPin(2);
        _doc["data"]["digital"]["4"] = this->iotaapCore.misc.getPin(4);
        _doc["data"]["digital"]["12"] = this->iotaapCore.misc.getPin(12);
        _doc["data"]["digital"]["13"] = this->iotaapCore.misc.getPin(13);
        _doc["data"]["digital"]["14"] = this->iotaapCore.misc.getPin(14);
        _doc["data"]["digital"]["15"] = this->iotaapCore.misc.getPin(15);
        _doc["data"]["digital"]["16"] = this->iotaapCore.misc.getPin(16);
        _doc["data"]["digital"]["17"] = this->iotaapCore.misc.getPin(17);
        _doc["data"]["digital"]["26"] = this->iotaapCore.misc.getPin(26);
        _doc["data"]["digital"]["27"] = this->iotaapCore.misc.getPin(27);
        _doc["data"]["digital"]["32"] = this->iotaapCore.misc.getPin(32);
        _doc["data"]["digital"]["33"] = this->iotaapCore.misc.getPin(33);
        _doc["data"]["digital"]["34"] = this->iotaapCore.misc.getPin(34);
        _doc["data"]["digital"]["35"] = this->iotaapCore.misc.getPin(35);

        _doc["data"]["analog"]["16"] = this->iotaapCore.misc.adc.getValue(16);
        _doc["data"]["analog"]["17"] = this->iotaapCore.misc.adc.getValue(17);
        _doc["data"]["analog"]["32"] = this->iotaapCore.misc.adc.getValue(32);
        _doc["data"]["analog"]["33"] = this->iotaapCore.misc.adc.getValue(33);
        _doc["data"]["analog"]["34"] = this->iotaapCore.misc.adc.getValue(34);
        _doc["data"]["analog"]["35"] = this->iotaapCore.misc.adc.getValue(35);

        char payload[1024];

        serializeJson(_doc, payload);

        String topic = "/" + String(this->_mqttUsername) + "/devices/" + String(this->_deviceID) + "/states";
        char topicChar[256];
        topic.toCharArray(topicChar, sizeof(topicChar));

        this->iotaapCore.mqtt.publish(topicChar, payload, false);
    }
}

/**
 * @brief Inner function to be used in MQTT callback as direct API listener on topic 'api/listen/<control_topic>'.
 * 
 * @param topic Callback topic parameter
 * @param message Callback message parameter
 * @param length Callback length parameter
 */
void IoTaaP_HAPI::callbackInnerFunction(char *topic, byte *message, unsigned int length)
{
    String messageTemp;
    DynamicJsonDocument inputBuffer(1024);

    for (int i = 0; i < length; i++)
    {
        messageTemp += (char)message[i];
    }

    String listenTopic = "/" + String(this->_mqttUsername) + "/devices/" + String(this->_deviceID) + "/listen";

    // Check if data is received on state layer
    if (String(topic) == listenTopic)
    {
        deserializeJson(inputBuffer, messageTemp);

        // Check if data type is output
        if (inputBuffer.containsKey("output"))
        {
            // Make pin as output
            this->iotaapCore.pwm.disablePWM((int)inputBuffer["output"]["pin"]);
            this->iotaapCore.misc.makePinOutput((int)inputBuffer["output"]["pin"]);
            // Check if pin is digital
            if (inputBuffer["output"]["type"] == "digital")
            {
                // Set or Clear the pin
                (int)inputBuffer["output"]["value"] ? this->iotaapCore.misc.setPin((int)inputBuffer["output"]["pin"]) : this->iotaapCore.misc.clearPin((int)inputBuffer["output"]["pin"]);
            }

            // Check if pin is analog
            if (inputBuffer["output"]["type"] == "analog")
            {
                // Set Analog value
                this->iotaapCore.pwm.setup(1, 5000, 16, (int)inputBuffer["output"]["pin"]);
                this->iotaapCore.pwm.set(1, (int)inputBuffer["output"]["value"]);
            }
        }
    }
}

/**
 * @brief Sets internal clock for CA certificate verification
 * 
 */
void IoTaaP_HAPI::setClock()
{
    configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // UTC

    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2)
    {
        yield();
        delay(500);
        now = time(nullptr);
    }
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
}

/**
 * @brief Checks if new firmware version is available on the server (periodically)
 * If new version is different then the current OTA update will be triggered
 * 
 */
void IoTaaP_HAPI::checkUpdate()
{
    DynamicJsonDocument versionJson(128);
    this->_client.setCACert(iotaap_ota_certificate);
    this->_client.setTimeout(12000 / 1000); // timeout argument is defined in seconds for setTimeout
    this->_httpClient.begin("https://ota.iotaap.io/v1/ota/device/latest/" + String(this->_deviceID), iotaap_ota_certificate);

    int httpCode = this->_httpClient.GET();
    Serial.println(httpCode);

    if (httpCode == 200)
    {
        String payload = this->_httpClient.getString();
        deserializeJson(versionJson, payload);

        String version = versionJson["ver"];

        if (version != this->_fwVersion)
        {
            this->otaUpdate();
        }
        else
        {
            this->_httpClient.end();
            return;
        }
    }
    else
    {
        this->_httpClient.end();
        return;
    }
}

/**
 * @brief Runs OTA Update and restarts MCU if successfull
 * 
 */
void IoTaaP_HAPI::otaUpdate()
{
    httpUpdate.setLedPin(ONBOARD_LED1, HIGH);
    httpUpdate.rebootOnUpdate(true);
    t_httpUpdate_return ret = httpUpdate.update(this->_client, "https://ota.iotaap.io/v1/ota/device/download/" + String(this->_deviceID) + String(this->_deviceToken));

    /*     switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    } */
}