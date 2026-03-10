# API Contract (v1)

## Gesture command envelope (Jetson -> Arduino)

JSON line messages:

```json
{"type":"gesture_start","name":"fingers_wave"}
{"type":"gesture_stop","name":"fingers_wave"}
```

## Allowed gesture names (default)

- `fingers_wave`
- `fingers_point`
- `fingers_open`
- `fingers_close`

## Safety policy

- Arm commands are rejected while `safety.enable_main_arms=false`.
- Finger gestures only are accepted in v1.

## Runtime flow

1. `SpeechPort.speak()` starts
2. `FacePort.set_expression()` to planned expression
3. `GesturePort.start_gesture()`
4. Speech playback
5. `GesturePort.stop_gesture()`
6. `FacePort.set_expression(neutral)`
7. Session metadata persisted through `StoragePort`

---

# Planned: API Contract (v2) — Main Arm Servo Extension

## Architecture

Hexagonal ports & adapters pattern:
- **Ports** (interfaces): `GesturePort`, `SpeechPort`, `FacePort`, `ASRPort`, `LLMPort`, `WheelPort`, `LocalizationPort`, `PerceptionPort`, `StoragePort`
- **Adapters** (implementations): Arduino serial, Riva ASR/TTS, Bedrock LLM, LCD stub, stubs for wheels/lidar/Kinect

## Hardware

Two ATmega2560 boards:
- **Board 1** (fingers): `finger_servos.ino` → `/dev/ttyUSB0` or `/dev/ttyUSB1`
- **Board 2** (main arms): `MainServoDirectControl.ino` → `/dev/ttyACM0`
  - Servos: `L_SH1`, `L_SH2`, `L_ELB`, `R_SH1`, `R_SH2`, `R_ELB`

## Planned gesture command envelope (v2)

```json
{"type":"arm_move","servo":"L_SH1","direction":"forward","duration_ms":300}
{"type":"arm_move","servo":"L_ELB","direction":"backward","duration_ms":200}
{"type":"arm_stop","servo":"L_SH1"}
```

## Allowed servo names (v2)

- `L_SH1` — Left shoulder 1 (continuous)
- `L_SH2` — Left shoulder 2 (continuous)
- `L_ELB` — Left elbow (positional)
- `R_SH1` — Right shoulder 1 (continuous)
- `R_SH2` — Right shoulder 2 (continuous)
- `R_ELB` — Right elbow (positional)

## Safety policy (v2)

- Main arm commands only accepted when `safety.enable_main_arms: true`
- Only one servo enabled at a time (enforced by adapter)
- `direction` must be exactly one of `forward` or `backward`

## Planned adapter

`robot_sync_app/adapters/gesture/arduino_arm_serial.py` — implements `GesturePort`, targets Board 2
