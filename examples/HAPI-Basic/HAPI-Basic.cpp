#include <IoTaaP.h>
#include "IoTaaP_HAPI.h"
#include <Adafruit_I2CDevice.h> // Temporary workaround for https://community.platformio.org/t/adafruit-gfx-lib-will-not-build-any-more-pio-5/15776 (2021/01/27) 

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