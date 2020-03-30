#include "IoTaaP.h"
#include "IoTaaP_HAPI.h"

IoTaaP iotaap;
IoTaaP_HAPI hapi("1.8.2");

void callback(char *topic, byte *message, unsigned int length)
{
}

void setup()
{

    Serial.begin(115200);

    iotaap.wifi.connect("SSID", "PASSWORD");
    hapi.configure("DEVICE_ID", "DEVICE_TOKEN", "MQTT_SERVER", "MQTT_USERNAME", "MQTT_PASSWORD", callback);
}

void loop()
{
    hapi.apiLoop();
}