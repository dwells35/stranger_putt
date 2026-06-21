#include <Arduino.h>
#include <FastLED.h>
#include "HoleNode.h"
#include "LEDPatterns.h"

#define LED_PIN      13
#define NUM_LEDS     60
#define LED_TYPE     WS2811
#define COLOR_ORDER  GRB
#define GOAL_PATTERN_DURATION_MS 3000

CRGB leds[NUM_LEDS];
HoleNode node("hole1");
LEDPatterns patterns(leds, NUM_LEDS);

static bool anim_running = false;
static bool full_on = false;
static u32_t previous_millis;

void onMqttMessage(String& topic, String& payload) {
    if (payload == "start") {
        full_on = true;
        fill_solid(leds, NUM_LEDS, CRGB::Cyan);
        FastLED.show();
    } else if (payload == "stop") {
        full_on = false;
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
    }
    else if (payload == "goal")
    {
        anim_running = true;
        previous_millis = millis();
    }
}

void setup() {
    Serial.begin(115200);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
           .setCorrection(TypicalLEDStrip);
    FastLED.setMaxRefreshRate(60);
    FastLED.setBrightness(64);

    node.onMessage(onMqttMessage);
    node.subscribe("golf/hole1/control");
    node.begin();
}

void loop() {
    node.loop();
    if (full_on && anim_running && ((millis() - previous_millis) < GOAL_PATTERN_DURATION_MS))
    {
        patterns.chase(CRGB::Cyan);
    } else if (full_on && anim_running) {
        anim_running = false;
        fill_solid(leds, NUM_LEDS, CRGB::Cyan);
        FastLED.show();
    }
}
