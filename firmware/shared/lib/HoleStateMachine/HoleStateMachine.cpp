#include "HoleStateMachine.h"

HoleStateMachine::HoleStateMachine(HoleNode& node, const char* holeId,
                                   uint32_t completeDurationMs,
                                   uint32_t resetDurationMs)
    : _node(node), _holeId(holeId), _state(HoleState::IDLE),
      _completeDurationMs(completeDurationMs), _resetDurationMs(resetDurationMs),
      _stateEnteredMs(0), _runStartMs(0), _eventCounter(0) {
    snprintf(_eventTopic, sizeof(_eventTopic), "golf/%s/event", holeId);
    strncpy(_playerId, "anonymous", sizeof(_playerId));
}

void HoleStateMachine::begin() {
    transitionTo(HoleState::IDLE);
}

void HoleStateMachine::loop() {
    uint32_t elapsed = millis() - _stateEnteredMs;

    if (_state == HoleState::COMPLETE && elapsed >= _completeDurationMs) {
        transitionTo(HoleState::RESET);
    } else if (_state == HoleState::RESET && elapsed >= _resetDurationMs) {
        transitionTo(HoleState::IDLE);
    }
}

void HoleStateMachine::registerPlayer(const char* playerId) {
    if (_state != HoleState::IDLE) return;
    strncpy(_playerId, playerId, sizeof(_playerId) - 1);
    _playerId[sizeof(_playerId) - 1] = '\0';
    transitionTo(HoleState::READY);
}

void HoleStateMachine::startGame() {
    if (_state != HoleState::READY) return;
    _runStartMs = millis();
    transitionTo(HoleState::RUN);
}

void HoleStateMachine::completeGame() {
    if (_state != HoleState::RUN) return;
    publishCompletionEvent();
    transitionTo(HoleState::COMPLETE);
}

void HoleStateMachine::playerDied() {
    if (_state != HoleState::RUN) return;
    if (_onPlayerDied) _onPlayerDied();
}

void HoleStateMachine::fault() {
    transitionTo(HoleState::FAULT);
}

void HoleStateMachine::reset() {
    transitionTo(HoleState::RESET);
}

void HoleStateMachine::transitionTo(HoleState next) {
    _state = next;
    _stateEnteredMs = millis();

    switch (next) {
        case HoleState::IDLE:      if (_onIdle)      _onIdle();      break;
        case HoleState::READY:     if (_onReady)     _onReady();     break;
        case HoleState::RUN:       if (_onRun)       _onRun();       break;
        case HoleState::COMPLETE:  if (_onComplete)  _onComplete();  break;
        case HoleState::RESET:     if (_onReset)     _onReset();     break;
        case HoleState::FAULT:     if (_onFault)     _onFault();     break;
    }
}

void HoleStateMachine::publishCompletionEvent() {
    char eventId[40];
    snprintf(eventId, sizeof(eventId), "%s-%lu", _holeId, ++_eventCounter);

    char payload[192];
    snprintf(payload, sizeof(payload),
        "{\"event\":\"completed\","
        "\"player_id\":\"%s\","
        "\"duration_ms\":%lu,"
        "\"timestamp\":%lu,"
        "\"event_id\":\"%s\"}",
        _playerId,
        millis() - _runStartMs,
        millis(),
        eventId);

    _node.publish(_eventTopic, payload);
}

void HoleStateMachine::onEnterIdle(void (*cb)())      { _onIdle = cb; }
void HoleStateMachine::onEnterReady(void (*cb)())     { _onReady = cb; }
void HoleStateMachine::onEnterRun(void (*cb)())        { _onRun = cb; }
void HoleStateMachine::onEnterComplete(void (*cb)())   { _onComplete = cb; }
void HoleStateMachine::onPlayerDied(void (*cb)())      { _onPlayerDied = cb; }
void HoleStateMachine::onEnterReset(void (*cb)())     { _onReset = cb; }
void HoleStateMachine::onEnterFault(void (*cb)())     { _onFault = cb; }
