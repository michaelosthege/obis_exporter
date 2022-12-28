#include "EEPROM.h"
#include <IotWebConf.h>
#include <list>
#include "ObisSensor.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

std::list<ObisSensor *> *sensors = new std::list<ObisSensor *>();
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf("OBISExporter", &dnsServer, &server, "", "1.0.3");

void wifiConnected()
{
    Serial.println("WiFi connected");
}
void configSaved()
{
    Serial.println("Config saved. Rebooting...");
    delay(1000);
    ESP.restart();
}
void handleMetrics()
{
    Serial.println("GET /metrics");
    String metrics = "";
    for (std::list<ObisSensor*>::iterator sens = sensors->begin(); sens != sensors->end(); ++sens)
    {
        metrics += render((*sens)->gauges);
    }
    server.send(200, "text/plain", metrics);
    Serial.println(metrics);
}


void setup()
{
    // Set up debugging via serial
    Serial.begin(115200);

    // Configure IR sensors
    Serial.println("Setting up sensors");
    sensors->push_back(new ObisSensor(D2));
    sensors->push_back(new ObisSensor(D5));

    // Setup WiFi and config stuff
    Serial.println("Setting up WiFi and web config");
    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);

    boolean validConfig = iotWebConf.init();
    if (!validConfig)
    {
        Serial.println("Missing or invalid config.");
    }

    server.on("/metrics", handleMetrics);
    server.on("/", []() { iotWebConf.handleConfig(); });
    server.onNotFound([]() { iotWebConf.handleNotFound(); });

    Serial.println("Setup completed.");
}

void loop()
{
    // Execute sensor state machines
    for (std::list<ObisSensor*>::iterator it = sensors->begin(); it != sensors->end(); ++it)
    {
        (*it)->loop();
    }
    iotWebConf.doLoop();
    yield();
}
