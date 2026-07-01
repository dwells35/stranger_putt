#pragma once
#include <FastLED.h>

class LEDPatterns {
public:
    LEDPatterns(CRGB* leds, uint16_t numLeds);
    void chase(CRGB color, uint8_t speed = 30);
    void spacedChase(CRGB color, uint8_t spacing = 3, uint16_t stepMs = 167);

private:
    CRGB* _leds;
    uint16_t _numLeds;
    uint16_t _pos;
    uint32_t _lastUpdate;
};
