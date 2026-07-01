#pragma once
#include "HoleNode.h"

enum class HoleState {
    IDLE,
    READY,
    RUN,
    COMPLETE,
    RESET,
    FAULT
};

class HoleStateMachine {
public:
    HoleStateMachine(HoleNode& node, const char* holeId,
                     uint32_t completeDurationMs = 5000,
                     uint32_t resetDurationMs    = 2000);

    void begin();
    void loop();

    // Transitions — call from sensor callbacks or game logic
    void registerPlayer(const char* playerId = "anonymous");
    void startGame();
    void completeGame();
    void playerDied();
    void fault();
    void reset();

    HoleState getState() const { return _state; }

    // One-shot callbacks fired on state entry
    void onEnterIdle(void (*cb)());
    void onEnterReady(void (*cb)());
    void onEnterRun(void (*cb)());
    void onEnterComplete(void (*cb)());
    void onPlayerDied(void (*cb)());
    void onEnterReset(void (*cb)());
    void onEnterFault(void (*cb)());

private:
    void transitionTo(HoleState next);
    void publishCompletionEvent();

    HoleNode&  _node;
    const char* _holeId;
    HoleState   _state;

    uint32_t _completeDurationMs;
    uint32_t _resetDurationMs;
    uint32_t _stateEnteredMs;
    uint32_t _runStartMs;
    uint32_t _eventCounter;

    char _playerId[32];
    char _eventTopic[32];

    void (*_onIdle)()      = nullptr;
    void (*_onReady)()     = nullptr;
    void (*_onRun)()       = nullptr;
    void (*_onComplete)()  = nullptr;
    void (*_onPlayerDied)() = nullptr;
    void (*_onReset)()     = nullptr;
    void (*_onFault)()     = nullptr;
};
