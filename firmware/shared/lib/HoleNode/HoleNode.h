#pragma once
#include <WiFi.h>
#include <MQTT.h>

class HoleNode {
public:
    HoleNode(const char* holeId);
    void begin();
    void loop();
    bool publish(const char* topic, const char* payload);
    void onMessage(MQTTClientCallbackSimple cb);
    void subscribe(const char* topic);

private:
    void connectWiFi();
    void connectMQTT();

    const char* _holeId;
    char _statusTopic[32];
    const char* _subTopic = nullptr;
    WiFiClient _net;
    MQTTClient _mqtt;
};
