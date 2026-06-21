# High-Tech Mini Golf — Project Brief

## Overview
A 4-hole miniature golf course where each hole is a co-operative 2-player puzzle:
one player's actions directly influence the other player's side of the hole
(e.g. lever/sensor on one side gates an LED path, motor, or obstacle on the
other side). Each hole is **fully self-contained** — holes never need to
communicate with each other. A central **scorekeeper** service passively
records completions across all holes for a leaderboard/dashboard.

## Hardware
- **Holes 2–4:** ESP32(s) only. Each hole reads sensors (the exact mix
  varies per hole) and drives LEDs / motors / other actuators in response.
  Some holes may need 2 ESP32 boards (e.g. one per player station) — not
  yet finalized per-hole, decide during that hole's design.
- **Hole 1:** ESP32 *plus* a separate Linux-based computer (could be a Raspberry Pi but doesn't have to be). The Pi handles heavier compute
  for this hole specifically (e.g. anything beyond simple sensor→actuator
  logic — vision, more complex state, etc.). The ESP32 still owns direct
  sensor/actuator I/O; the Pi is local compute for hole 1 only, not a
  general architecture component.
- **LEDs:** Dense runs, 200+ addressable LEDs per hole. This is a real
  power and signal-integrity concern, not just a code concern (see below).

## Firmware Stack
- **Build system: PlatformIO, using the Arduino framework** (not raw
  ESP-IDF, not the Arduino IDE itself).
  - Rationale: need fast iteration across 4 structurally different holes,
    want to leverage the mature Arduino-ecosystem library set (FastLED,
    PubSubClient, sensor libraries), and want a real project structure
    (proper library deps, per-environment `platformio.ini` configs,
    CLI/CI-friendly builds) rather than copy-pasted sketch folders.
  - ESP-IDF was considered and rejected for this project: we don't have
    the resource constraints, latency requirements, or need for
    fine-grained FreeRTOS control that would justify the dev-velocity
    cost. Can mix in ESP-IDF components later for a specific hole if a
    real need arises — not a blocker either way.
- **LED library: FastLED**, using the RMT peripheral driver on ESP32
  (FastLED 3.6+, pin core/board versions deliberately — ESP32 RMT support
  has had breakage across Arduino-core/ESP-IDF version bumps historically).
  - At 200+ pixels/hole: budget power carefully. WS2812B pixels draw up to
    ~60mA each at full white → a 200-pixel strip can pull ~12A at 5V.
    Use a dedicated 5V supply (not the ESP32's onboard regulator), inject
    power at multiple points along long runs, and level-shift the data
    line (3.3V data over long WS2812 runs gets unreliable).
  - Consider splitting one hole's animation across multiple parallel
    strips/data pins (FastLED supports multiple parallel RMT outputs)
    rather than one very long daisy chain — reduces per-frame latency and
    isolates a bad connector to one segment.
  - Frame budget: ~30µs/pixel to shift out WS2812 data, so a single
    200-pixel strip takes ~6ms/frame — fine for ~60fps, but be aware this
    competes with Wi-Fi/MQTT work on the same core.

## Networking
- **Protocol: MQTT over Wi-Fi**, broker = **Mosquitto** (run on a Pi or
  small always-on box on-site).
  - Considered and rejected Zenoh: Zenoh has real advantages (lower
    latency, no required broker, better fit for high-frequency telemetry
    and querying), but none of those advantages are exercised by this
    project's actual requirements (low message rate, discrete one-way
    events, no inter-hole comms, no need for the scorekeeper to query
    live state). MQTT's mature ecosystem and "boring but reliable"
    operational profile (Mosquitto as an unattended appliance) outweigh
    Zenoh's architectural benefits here. Reassess only if requirements
    change (e.g. high-frequency telemetry, inter-hole comms, removing
    the broker as a single point of failure becomes a real concern at
    higher hole counts).
  - **ESP32 MQTT client library:** PubSubClient (or arduino-mqtt) —
    either is fine, PubSubClient is the more common default.
- **Communication is one-way: hole → scorekeeper only.** Scorekeeper never
  sends commands back to holes (no remote reset/diagnostics from the
  dashboard, by design — holes are fully autonomous). If this changes
  later it's an additive change (new topic + new subscriber on the
  ESP32 side), not a redesign.
- **Topic structure:**
  ```
  golf/hole1/event     -> {"event": "completed", "player_id": "...", "duration_ms": ..., "timestamp": ..., "event_id": "..."}
  golf/hole1/status    -> "online" | "offline"   (retained, set via MQTT Last Will and Testament)
  golf/hole2/event
  golf/hole2/status
  golf/hole3/event
  golf/hole3/status
  golf/hole4/event
  golf/hole4/status
  ```
  - `status` topics are **retained** (so a newly-connecting dashboard
    immediately knows current online/offline state) and use MQTT's LWT
    feature so a board losing power/Wi-Fi automatically flips to
    "offline" with zero extra code.
  - `event` topics are **NOT retained** (avoid replaying a stale
    "completed" event and double-counting a score if the scorekeeper
    restarts).
  - Every event carries a unique `event_id` so the scorekeeper can
    de-duplicate — MQTT QoS 1 can redeliver the same message if a flaky
    Wi-Fi moment causes a retry.

## Game Logic Placement
- **Game-completion / win-condition logic lives on the ESP32 itself**,
  per hole. The ESP32 owns the full local state machine
  (idle → in_progress → won/failed → resetting) and only publishes a
  single authoritative event when state changes.
- **The scorekeeper is a pure, subscribe-only sink.** It has zero
  knowledge of any hole's game rules — it just ingests `golf/+/event`
  and `golf/+/status`, persists to a database, and serves a
  dashboard/leaderboard. This keeps holes fully autonomous and keeps the
  scorekeeper's risk surface minimal (no path by which a dashboard bug
  could affect hardware, since it's read-only).

## Project Structure
```
mini-golf/
├── firmware/                       # PlatformIO project(s) for all ESP32 boards
│   ├── shared/
│   │   ├── lib/
│   │   │   ├── HoleNode/           # WiFi connect/reconnect, MQTT publish-only client,
│   │   │   │                       # heartbeat/status + LWT, OTA support
│   │   │   ├── HoleStateMachine/   # idle -> in_progress -> won/failed -> resetting,
│   │   │   │                       # publishes event on transition
│   │   │   ├── SensorUtils/        # debouncing, IR break-beam helpers, etc.
│   │   │   └── LEDPatterns/        # reusable FastLED animations (chase, fade, etc.)
│   │   └── config/
│   │       └── secrets.h.example   # wifi creds, broker IP/port — actual secrets.h gitignored
│   └── holes/
│       ├── hole1_node/             # ESP32 firmware that pairs with hole1's Linux computer
│       ├── hole2_node/             # (and hole2_node_b/ etc. if a hole needs 2 boards)
│       ├── hole3_node/
│       └── hole4_node/
├── scorekeeper/
│   ├── server/
│   │   ├── mqtt_listener.py        # subscribes golf/+/event, golf/+/status
│   │   ├── score_engine.py         # de-dup via event_id, persistence
│   │   ├── db/                     # SQLite is sufficient at this scale
│   │   └── api.py                  # REST/WebSocket for the dashboard
│   └── dashboard/                  # leaderboard + hole online/offline status UI
└── tools/
    └── ota_flash_all.py            # batch flash/update helper across boards
```

## Open / Not-Yet-Decided
- Exact sensor/actuator mix and win condition per hole (2, 3, 4) — design
  per-hole as we get to them.
- Whether any hole beyond hole 1 needs 2 ESP32 boards instead of 1.
- Exact LED layout/segmentation per hole (how many parallel strips/pins).
- Where Mosquitto broker physically runs (hole 1's Pi vs. a separate
  always-on box) — leaning toward separate box to avoid coupling broker
  uptime to hole 1's Pi, but not finalized.
