#include <Arduino.h>
#include <FastLED.h>
#include <SPI.h>
#include <MFRC522.h>
#include "HoleNode.h"
#include "HoleStateMachine.h"
#include "LEDPatterns.h"

#define LED_PIN      15
#define NUM_LEDS     60
#define LED_TYPE     WS2811
#define COLOR_ORDER  BRG

#define RFID_SS_PIN  5
#define RFID_RST_PIN 22

#define COMPLETE_DURATION_MS 3000

CRGB leds[NUM_LEDS];
HoleNode node("hole1");
HoleStateMachine hsm(node, "hole1", COMPLETE_DURATION_MS);
LEDPatterns patterns(leds, NUM_LEDS);
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

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

void onRun() {
    fill_solid(leds, NUM_LEDS, CRGB::Cyan);
    FastLED.show();
}

void onReset() {
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

// Reads a present card's UID into uid[] as a colon-separated hex string.
// Returns the number of bytes read (0 if no card or read failed).
static uint8_t readUid(char* uid, size_t uidLen) {
    if (!rfid.PICC_IsNewCardPresent()) return 0;
    if (!rfid.PICC_ReadCardSerial())   return 0;

    uid[0] = '\0';
    for (byte i = 0; i < rfid.uid.size; i++) {
        char hex[4];
        snprintf(hex, sizeof(hex), i == 0 ? "%02X" : ":%02X", rfid.uid.uidByte[i]);
        strncat(uid, hex, uidLen - strlen(uid) - 1);
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return rfid.uid.size;
}

void setup() {
    Serial.begin(115200);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
           .setCorrection(TypicalLEDStrip);
    FastLED.setMaxRefreshRate(60);
    FastLED.setBrightness(64);

    SPI.begin();
    rfid.PCD_Init();
    Serial.println("RFID reader ready");

    hsm.onEnterIdle(onIdle);
    hsm.onEnterReady(onReady);
    hsm.onEnterRun(onRun);
    hsm.onEnterReset(onReset);
    hsm.onEnterFault(onFault);

    node.onMessage(onMqttMessage);
    node.subscribe("golf/hole1/control");
    node.begin();
    hsm.begin();
}

void loop() {
    node.loop();
    hsm.loop();

    if (hsm.getState() == HoleState::IDLE) {
        char uid[24];
        if (readUid(uid, sizeof(uid))) {
            Serial.printf("Tag detected: %s\n", uid);
            hsm.registerPlayer(uid);
        }
    }

    HoleState state = hsm.getState();
    if (state == HoleState::IDLE) {
        patterns.spacedChase(CRGB::Cyan);
    } else if (state == HoleState::COMPLETE) {
        patterns.chase(CRGB::Cyan);
    }
}
