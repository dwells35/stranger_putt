#include "LEDPatterns.h"

LEDPatterns::LEDPatterns(CRGB* leds, uint16_t numLeds)
    : _leds(leds), _numLeds(numLeds), _pos(0), _lastUpdate(0) {}

void LEDPatterns::chase(CRGB color, uint8_t speed) {
    uint32_t now = millis();
    if (now - _lastUpdate < speed) return;
    _lastUpdate = now;

    fadeToBlackBy(_leds, _numLeds, 64);
    _leds[_pos] = color;
    _pos = (_pos + 1) % _numLeds;
    FastLED.show();
}
