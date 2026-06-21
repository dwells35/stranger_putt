#include "HoleNode.h"
#include "secrets.h"

HoleNode::HoleNode(const char* holeId) : _holeId(holeId) {
    snprintf(_statusTopic, sizeof(_statusTopic), "golf/%s/status", holeId);
}

void HoleNode::begin() {
    connectWiFi();
    _mqtt.begin(MQTT_BROKER_IP, MQTT_BROKER_PORT, _net);
    connectMQTT();
}

void HoleNode::loop() {
    if (!_mqtt.connected()) connectMQTT();
    _mqtt.loop();
}

bool HoleNode::publish(const char* topic, const char* payload) {
    return _mqtt.publish(topic, payload);
}

void HoleNode::onMessage(MQTTClientCallbackSimple cb) {
    _mqtt.onMessage(cb);
}

void HoleNode::subscribe(const char* topic) {
    _subTopic = topic;
    if (_mqtt.connected()) _mqtt.subscribe(topic);
}

void HoleNode::connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);
}

void HoleNode::connectMQTT() {
    char clientId[24];
    snprintf(clientId, sizeof(clientId), "hole-%s", _holeId);

    // LWT: flip status to "offline" if connection drops unexpectedly
    _mqtt.setWill(_statusTopic, "offline", true, 0);

    while (!_mqtt.connect(clientId)) {
        delay(2000);
    }

    _mqtt.publish(_statusTopic, "online", true, 0);
    if (_subTopic) _mqtt.subscribe(_subTopic);
}
