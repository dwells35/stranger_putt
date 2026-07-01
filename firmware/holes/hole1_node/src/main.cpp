#include <Arduino.h>
#include <FastLED.h>
#include "HoleNode.h"
#include "HoleStateMachine.h"
#include "LEDPatterns.h"

#define LED_PIN      15
#define NUM_LEDS     60
#define LED_TYPE     WS2811
#define COLOR_ORDER  BRG

// How long the completion animation plays before resetting
#define COMPLETE_DURATION_MS 3000

CRGB leds[NUM_LEDS];
HoleNode node("hole1");
HoleStateMachine hsm(node, "hole1", COMPLETE_DURATION_MS);
LEDPatterns patterns(leds, NUM_LEDS);

// --- State-enter callbacks (one-shot on each transition) ---

void onIdle() {
    // Animation runs in loop(); nothing to do on entry
}

void onReady() {
    for (int i = 0; i < 3; i++) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        delay(50);
        fill_solid(leds, NUM_LEDS, CRGB::Green1);
        FastLED.show();
        delay(150);
    }
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

void onRunning() {
    fill_solid(leds, NUM_LEDS, CRGB::Cyan);
    FastLED.show();
}

void onResetting() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

void onFault() {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
}

// --- MQTT command → state machine ---
//
//   register → IDLE→READY       (player steps up)
//   start    → READY→RUNNING    (game begins)
//   goal     → RUNNING→COMPLETE (hole scored)
//   fault    → any→FAULT        (emergency stop)
//   reset    → any→RESETTING    (admin reset)

void onMqttMessage(String& topic, String& payload) {
    if      (payload == "register") hsm.registerPlayer();
    else if (payload == "start")    hsm.startGame();
    else if (payload == "goal")     hsm.completeGame();
    else if (payload == "fault")    hsm.fault();
    else if (payload == "reset")    hsm.reset();
}

void setup() {
    Serial.begin(115200);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
           .setCorrection(TypicalLEDStrip);
    FastLED.setMaxRefreshRate(60);
    FastLED.setBrightness(64);

    hsm.onEnterIdle(onIdle);
    hsm.onEnterReady(onReady);
    hsm.onEnterRunning(onRunning);
    hsm.onEnterResetting(onResetting);
    hsm.onEnterFault(onFault);

    node.onMessage(onMqttMessage);
    node.subscribe("golf/hole1/control");
    node.begin();
    hsm.begin();
}

void loop() {
    node.loop();
    hsm.loop();

    HoleState state = hsm.getState();
    if (state == HoleState::IDLE) {
        patterns.spacedChase(CRGB::Cyan);
    } else if (state == HoleState::COMPLETE) {
        patterns.chase(CRGB::Cyan);
    }
}
