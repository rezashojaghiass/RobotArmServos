# Robot Arm Servos Control

Arduino-based control system for a humanoid robot with dual articulated hands, arm movements, and omnidirectional wheel base.

## Project Location

- **Primary Workspace**: `/home/reza/RobotArmServos`
- **Backup Workspace**: `/mnt/nvme/RobotArmServos`
- **GitHub Repository**: https://github.com/rezashojaghiass/RobotArmServos

---

## Table of Contents

1. [Overview](#overview)
2. [Where to Find What](#where-to-find-what)
3. [Hardware Requirements](#hardware-requirements)
4. [Pin Mapping](#pin-mapping)
5. [Software Requirements](#software-requirements)
6. [Project Files](#project-files)
7. [Usage](#usage)
8. [Calibration](#calibration)
9. [Timing Configuration](#timing-configuration)
10. [Troubleshooting](#troubleshooting)
11. [Performance Optimization](#performance-optimization)
12. [Safety Features](#safety-features)
13. [Example Sequences](#example-sequences)
14. [Advanced Configuration](#advanced-configuration)
15. [Gesture Serial Incident (Root Cause + Fix)](#gesture-serial-incident-root-cause--fix)
16. [Future Enhancements](#future-enhancements)
17. [References](#references)
18. [License](#license)

---

## Where to Find What

| I want to… | Go to |
|---|---|
| **Upload a sketch to the arm/finger board** | [`AllServosFeb10.ino`](AllServosFeb10.ino) or [`MainServoDirectControl/`](MainServoDirectControl/) |
| **Test a single servo in isolation** | [`MainServoDirectControl/MainServoDirectControl.ino`](MainServoDirectControl/MainServoDirectControl.ino) |
| **Control fingers only** | [`ArduinoServoControl.ino`](ArduinoServoControl.ino) or [`Humanoid/arduino/finger_servos/`](Humanoid/arduino/finger_servos/) |
| **Control the wheel base** | [`OmnidirectionalWheelControl/`](OmnidirectionalWheelControl/) |
| **Understand the software API / port design** | [`robot_sync_app/docs/API_CONTRACT.md`](robot_sync_app/docs/API_CONTRACT.md) |
| **Run the robot_sync_app (main Python app)** | [`robot_sync_app/README.md`](robot_sync_app/README.md) |
| **See all hardware pin assignments** | [`Humanoid/hardware/pinouts/PIN_MAPPING.md`](Humanoid/hardware/pinouts/PIN_MAPPING.md) |
| **Calibrate servos** | [`Humanoid/hardware/calibration/CALIBRATION.md`](Humanoid/hardware/calibration/CALIBRATION.md) |
| **Understand hardware specs** | [`Humanoid/hardware/specs/SPECIFICATIONS.md`](Humanoid/hardware/specs/SPECIFICATIONS.md) |
| **Set up voice chat / Riva ASR+TTS** | [`Humanoid/references/chatbot_robot/VOICE_SETUP.md`](Humanoid/references/chatbot_robot/VOICE_SETUP.md) |
| **Set up LiDAR (Unitree)** | [`Humanoid/references/lidar_unitree/LIDAR_ROS2_GUIDE.md`](Humanoid/references/lidar_unitree/LIDAR_ROS2_GUIDE.md) |
| **Understand facial animation** | [`Humanoid/references/facial_animation/ANIMATION_SYSTEM.md`](Humanoid/references/facial_animation/ANIMATION_SYSTEM.md) |
| **Set up remote display (VNC)** | [`Humanoid/references/vnc_setup/REMOTE_DISPLAY.md`](Humanoid/references/vnc_setup/REMOTE_DISPLAY.md) |
| **See the full Humanoid integration guide** | [`Humanoid/docs/INTEGRATION.md`](Humanoid/docs/INTEGRATION.md) |
| **Quick start the whole system** | [`Humanoid/QUICKSTART.md`](Humanoid/QUICKSTART.md) |
| **Understand gesture examples** | [`Humanoid/examples/gesture_patterns/GESTURES.md`](Humanoid/examples/gesture_patterns/GESTURES.md) |
| **PS2 library source** | [`PS2X_lib/`](PS2X_lib/) |

---

## Overview

This project controls a humanoid robot featuring:
- **Dual Hand System**: 10 servo-controlled fingers (5 per hand)
- **Arm Servos**: Shoulder and elbow joints for both arms
- **Omnidirectional Base**: 4-wheel mecanum/omni-directional drive
- **PS2 Wireless Control**: DualShock controller for mobile base control
- **Wave Animations**: Synchronized finger wave patterns with phase delays

## Hardware Requirements

### Microcontroller

### Dual Arduino Setup

This project uses **two Arduino Mega 2560 boards** for distributed control:

#### Arduino #1 - Fingers & Arms Control
- **Port**: `/dev/ttyACM0` (Linux) or COM port (Windows)
- **Type**: Official Arduino Mega 2560 R3
- **USB Chip**: ATmega16U2
- **Serial Number**: `343383235313515061E1`
- **USB Path**: `1-2.2`
- **Controls**: 10 finger servos + 6 arm servos (shoulders & elbows)
- **Sketch**: `ArduinoServoControl.ino` or `AllServosFeb10.ino`

#### Arduino #2 - Robot Base Control
- **Port**: `/dev/ttyUSB0` (Linux) or COM port (Windows)
- **Type**: Arduino Mega 2560 Clone
- **USB Chip**: CH340 (Vendor ID: `1a86`, Product ID: `7523`)
- **USB Path**: `1-4.3`
- **Controls**: 4 DC motors for omnidirectional movement
- **Sketch**: `OmnidirectionalWheelControl.ino`
- **Current Features**: 
  - Smooth acceleration/deceleration (RAMP_RATE = 1)
  - Speed ratio control (SPEED_RATIO = 0.1 = 45 PWM)
  - PS2 controller input for forward/backward/strafe/rotate

#### Identifying Your Boards

**By USB Chip (Most Reliable - Device Name):**
```bash
# Check USB device info
udevadm info -a -n /dev/ttyUSB0 | grep ATTRS{idProduct}
# Arduino #1 (Official): idProduct == 0042 (ATmega16U2)
# Arduino #2 (Clone): idProduct == 7523 (CH340)

# Or use lsusb
lsusb | grep Arduino
# Arduino #1: 2341:0042 Arduino SA Mega 2560 R3
# Arduino #2: 1a86:7523 (not in lsusb, but ID is CH340)
```

**By USB Path (Reliable but may change):**
```bash
ls -l /dev/serial/by-path/
# Arduino #1: *1-2.2*
# Arduino #2: *1-4.3*
```

**By Port Type:**
```bash
ls /dev/ttyACM* /dev/ttyUSB*
# Arduino #1: /dev/ttyACM0
# Arduino #2: /dev/ttyUSB0
```

**Upload to Specific Board:**
```bash
# Upload to Arduino #1 (Fingers & Arms) - Servo Control
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:mega /path/to/sketch

# Upload to Arduino #2 (Base Control) - Motor Control
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:mega /path/to/sketch
```

- **Arduino Mega 2560** (recommended due to pin count requirements)

- Alternative: Arduino Due or similar board with sufficient PWM pins

### Servos
- **10x Standard Servos**: Fingers (5 per hand)
- **6x Continuous/Positional Servos**: Arms (shoulders and elbows)
- **Power Requirements**: External 5V/6V servo power supply (5-10A recommended)

### Motors & Drivers
- **4x DC Motors**: Omnidirectional wheels
- **Motor Driver**: L298N or similar H-bridge (4 channels)
- **Motor Power**: 12V battery pack (2-5A)

### Controller
- **PS2 Wireless Controller**: DualShock compatible
- **PS2 Receiver Module**: Connected via SPI

### Power System
- Servo power: 5-6V, 5-10A supply
- Motor power: 12V, 2-5A battery
- Arduino power: USB or 7-12V barrel jack
- **6x Power Control Pins**: For servo safety (D30-D35)

## Pin Mapping

### Right Hand Fingers
```
Thumb:  D2
Index:  D3
Middle: D4
Ring:   D5
Pinky:  D6
```

### Left Hand Fingers
```
Thumb:  D7
Index:  D8
Middle: D9
Ring:   D10
Pinky:  D11
```

### Arm Servos (AllServosFeb10.ino)
```
LEFT ARM:
- Shoulder 1: Signal D17, Power D33
- Shoulder 2: Signal D25, Power D35
- Elbow:      Signal D24, Power D34

RIGHT ARM:
- Shoulder 1: Signal D14, Power D30
- Shoulder 2: Signal D15, Power D31
- Elbow:      Signal D16, Power D32
```

### Omnidirectional Motors
```
Motor A: PWM D11, DIR1 D34, DIR2 D35
Motor B: PWM D7,  DIR1 D36, DIR2 D37
Motor C: PWM D6,  DIR1 D43, DIR2 D42
Motor D: PWM D4,  DIR1 A5,  DIR2 A4
```

### PS2 Controller
```
DAT (Data):    D13 (or D9)
CMD (Command): D11 (or D10)
SEL (Select):  D10 (or D13)
CLK (Clock):   D12 (or D44)
```

## Software Requirements

### Arduino Libraries
```cpp
#include <Servo.h>        // Built-in Arduino Servo library
#include <PS2X_lib.h>     // Included in this repository
```

### Installation
1. Install Arduino IDE (1.8.x or 2.x)
2. Clone this repository
3. Copy `PS2X_lib` folder to Arduino libraries directory:
   - Windows: `Documents\Arduino\libraries\`
   - Mac: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`

## Project Files

### Main Control Programs

**ArduinoServoControl.ino**
- Controls both hands + left shoulder
- Synchronized wave animation
- 1-3 repeat cycles configurable
- Final position hold with correction

**AllServosFeb10.ino**
- Full arm control (shoulders + elbows, both sides)
- Independent left/right arm timing
- Configurable speed factors
- Enable/disable flags for each servo

**OmnidirectionalWheelControl.ino**
- 4-wheel omni-directional drive
- PS2 controller input
- Forward, backward, strafe, rotate movements
- Real-time serial feedback

### Library Files

**PS2X_lib/**
- PS2 controller communication library
- Examples included
- Supports wired and wireless controllers

**Servo.h/cpp**
- Custom servo library with interrupt-driven control
- Multi-architecture support (AVR, SAM, SAMD, etc.)

## Usage

### Hand Wave Animation

**Upload ArduinoServoControl.ino:**

```cpp
const int REPEAT_COUNT = 1;  // Number of wave cycles
const float SPEED = 1.8;     // Animation speed multiplier
```

**Behavior:**
1. All fingers start in OPEN position
2. Mexican wave pattern (thumb → pinky, right → left)
3. Left shoulder raises during wave
4. Returns to open position after cycles complete
5. Automatic power-off and detach

### Full Arm Control

**Upload AllServosFeb10.ino:**

```cpp
// Enable/disable individual servos
const bool EN_L_SH1 = true;  // Left shoulder 1
const bool EN_L_ELB = true;  // Left elbow
const bool EN_R_SH1 = true;  // Right shoulder 1

// Independent timing (milliseconds)
const unsigned long L_ARM_UP_MS    = 2000;
const unsigned long L_ARM_PAUSE_MS = 180;
const unsigned long L_ARM_DOWN_MS  = 1570;

const unsigned long R_ARM_UP_MS    = 2000;
const unsigned long R_ARM_PAUSE_MS = 180;
const unsigned long R_ARM_DOWN_MS  = 1900;

// Speed control (for continuous servos)
const float L_SH1_SPEED = 1.00f;
const float R_SH1_SPEED = 1.25f;  // Right arm faster
```

**Features:**
- Decoupled left/right arm movements
- Configurable timing per arm
- Speed multipliers for continuous servos
- Safe power-on/off sequence

### Mobile Base Control

**Upload OmnidirectionalWheelControl.ino:**

**PS2 Controller Mapping:**
- **Left Stick Y**: Forward/Backward
- **Left Stick X**: Strafe Left/Right
- **Right Stick X**: Rotate Left/Right

**Movements:**
```
Forward:  ⬆️  All motors forward
Backward: ⬇️  All motors reverse
Left:     ⬅️  A,C reverse, B,D forward
Right:    ➡️  A,C forward, B,D reverse
Rot Left: ↩️  A reverse, B,C,D forward
Rot Right:↪️  A,D forward, B,C reverse
```

## Calibration

### Servo Endpoints

**Right Hand (in microseconds):**
```cpp
THUMB_OPEN  = 1360-1500  THUMB_CLOSE  = 2300
INDEX_OPEN  = 2000       INDEX_CLOSE  = 700
MIDDLE_OPEN = 2000       MIDDLE_CLOSE = 700
RING_OPEN   = 2000       RING_CLOSE   = 700
PINKY_OPEN  = 2000       PINKY_CLOSE  = 700
```

**Left Hand (mirrored):**
```cpp
LTHUMB_OPEN  = 1930      LTHUMB_CLOSE  = 740
LINDEX_OPEN  = 700       LINDEX_CLOSE  = 2000
LMIDDLE_OPEN = 700       LMIDDLE_CLOSE = 2000
LRING_OPEN   = 700       LRING_CLOSE   = 2000
LPINKY_OPEN  = 700       LPINKY_CLOSE  = 2000
```

### Finding Your Servo Values

1. Use `writeMicroseconds()` with test values (600-2400)
2. Find OPEN position (fingers extended)
3. Find CLOSE position (fingers curled)
4. Update constants in code
5. Test wave motion for smooth operation

### Continuous Servo Neutral

**Shoulders (continuous rotation):**
```cpp
LSH1_NEUTRAL = 1370   // Adjust until motor stops
RSH1_NEUTRAL = 1460
```

Test by setting to neutral and verifying motor stops completely.

## Timing Configuration

### Wave Animation Timing
```cpp
const unsigned long PHASE_DELAY = 120;   // Delay between fingers (ms)
const unsigned long HALF_MOVE   = 800;   // Time for open→close or close→open
const unsigned long UPDATE_MS   = 20;    // Servo update interval
```

**Mexican Wave Effect:**
- Each finger starts 120ms after previous
- Total wave time: 10 fingers × 120ms = 1.2 seconds
- Each finger takes 1.6 seconds (800ms × 2) for full cycle

### Arm Movement Timing

```cpp
// Example: Left arm takes 3.75 seconds total
L_ARM_UP_MS    = 2000   // Raise arm (2.0s)
L_ARM_PAUSE_MS = 180    // Hold at top (0.18s)
L_ARM_DOWN_MS  = 1570   // Lower arm (1.57s)
```

## Troubleshooting

### Servos Not Moving

**Check:**
1. Power supply connected and sufficient amperage (5-10A)
2. Servo signal wires connected to correct pins
3. Ground common between Arduino and servo power supply
4. Power enable pins (D30-D35) set HIGH in code

**Solution:**
```cpp
pinMode(SERVO_PWR_PIN, OUTPUT);
digitalWrite(SERVO_PWR_PIN, HIGH);  // Enable power
delay(250);                         // Wait for power stabilization
```

### Erratic Servo Behavior

**Causes:**
- Insufficient power supply
- Loose connections
- EMI from motors
- Multiple servos moving simultaneously

**Solutions:**
- Use larger power supply (10A recommended)
- Add capacitors (1000-4700μF) across servo power rails
- Separate motor and servo power supplies
- Stagger servo movements in code

### PS2 Controller Not Connecting

**Check wiring:**
```cpp
// Verify pin connections match code
#define PS2_DAT  13
#define PS2_CMD  11
#define PS2_SEL  10
#define PS2_CLK  12
```

**Test connection:**
```cpp
error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);

if (error == 0) Serial.println("✅ Controller connected!");
else Serial.println("❌ Connection failed!");
```

**Common issues:**
- Wrong pin assignments
- Weak batteries in controller
- Receiver module not powered (3.3V or 5V)
- Interference from other devices

### Motors Running Continuously

**Continuous servo stuck:**
```cpp
// Find true neutral point
servo.writeMicroseconds(1460);  // Start here
// Adjust ±20 until motor stops: 1440, 1450, 1460, 1470, 1480
```

**Motor controller issue:**
- Check direction pins are OUTPUT
- Verify PWM value is 0 when stopped
- Test motor driver with separate sketch

### Fingers Don't Return to Open Position

**AllServosFeb10.ino specific:**

Check endpoint values are correct for your servos. Left hand is mirrored:

```cpp
// If left thumb closes at end, swap these:
const int LTHUMB_OPEN  = 1930;  // ← Should be extended
const int LTHUMB_CLOSE = 740;   // ← Should be curled
```

## Performance Optimization

### Memory Usage
- Each Servo object: ~10 bytes
- 16 servos: ~160 bytes
- Arduino Mega: 8KB SRAM (plenty of headroom)

### Timing Accuracy
- Uses hardware timers for servo PWM
- Update rate: 20ms (50Hz) per servo
- Jitter: <1ms typical

### Power Management
- Active servos: ~500mA each (under load)
- 16 servos: Up to 8A total
- Motors: 2-4A total
- **Recommended**: 10A servo supply + 5A motor supply

## Safety Features

### Power Control Pins
All arm servos have dedicated power enable pins:
```cpp
digitalWrite(SERVO_PWR_PIN, HIGH);  // Power ON
// ... movement code ...
digitalWrite(SERVO_PWR_PIN, LOW);   // Power OFF
```

### Neutral Position Before Power
```cpp
// SAFE SEQUENCE:
servo.writeMicroseconds(NEUTRAL);  // Set neutral first
delay(200);                        // Wait for signal
powerOn(PWR_PIN);                  // Then power ON
```

### Detach After Use
```cpp
servo.detach();  // Stop PWM signal
pinMode(pin, OUTPUT);
digitalWrite(pin, LOW);  // Pull pin LOW
```

## Example Sequences

### Simple Wave (Right Hand Only)
```cpp
// Set left hand to false, keep right hand servos
// Reduce REPEAT_COUNT to 1
// Upload ArduinoServoControl.ino
```

### Synchronized Arms
```cpp
// AllServosFeb10.ino
// Set L_ARM and R_ARM timings identical:
const unsigned long L_ARM_UP_MS = 2000;
const unsigned long R_ARM_UP_MS = 2000;
// Both arms move together
```

### Faster Wave
```cpp
const float SPEED = 2.5;  // 2.5x faster
// Or reduce timing values:
const unsigned long PHASE_DELAY = 80;   // Was 120
const unsigned long HALF_MOVE   = 600;  // Was 800
```

## Advanced Configuration

### Enable/Disable Servos

In `AllServosFeb10.ino`:
```cpp
const bool EN_L_SH1 = true;   // Enable left shoulder 1
const bool EN_L_SH2 = false;  // Disable left shoulder 2
const bool EN_L_ELB = true;   // Enable left elbow
// Disabled servos won't attach, no power, no movement
```

### Speed Multipliers (Continuous Servos)
```cpp
const float L_SH1_SPEED = 1.00f;  // Normal speed
const float R_SH1_SPEED = 1.25f;  // 25% faster

// Applied as:
int scaled = scaledPulse(NEUTRAL, TARGET, SPEED);
// Speed=1.0 → unchanged
// Speed=1.5 → 50% faster
// Speed=0.5 → 50% slower
```

## Gesture Serial Incident (Root Cause + Fix)

### Symptom
- During voice chat, gesture logs showed command send on the Python side, but the index finger did not move.

### Root cause
- Arduino did not reliably decode the live chat command line before motion dispatch.
- Evidence from serial logs showed line-buffer resets (`Input guard reset line buffer`) without decode matches (`Matched ...`) or acknowledgements (`ACK TALK_ON`).
- Because decode failed, `talking` never became `true`, so the finger motion loop was never entered.

### Fix implemented
- For `fingers_wave`, the Python adapter now sends short control frames (`TALK_ON` / `TALK_OFF`) instead of relying only on longer JSON.
- Adapter delivery was hardened with:
  - serial input flush before send,
  - immediate write flush,
  - retry once if no Arduino response is observed.
- Arduino sketch was instrumented with byte-level receive/decode logs (raw byte value, line boundaries, trim/compact lengths, and decode branch logs).

### Verification
- Added diagnostic script: [`Humanoid/examples/test_scripts/test_talk_motion_serial.py`](Humanoid/examples/test_scripts/test_talk_motion_serial.py)
- Added debug firmware path: [`Humanoid/arduino/finger_servos/talk_motion_serial.ino`](Humanoid/arduino/finger_servos/talk_motion_serial.ino)
- Verified sequence in logs:
  - sent bytes for `TALK_ON` and `TALK_OFF`,
  - Arduino byte-by-byte receive logs,
  - decode match (`Matched TALK_ON plain text` / `Matched TALK_OFF plain text`),
  - ACKs (`ACK TALK_ON` / `ACK TALK_OFF`),
  - index finger talk loop active only while speaking.

### Current behavior
- Voice mode now moves the right index finger during speech windows and stops/detaches on stop.

## Future Enhancements

- [ ] Add inverse kinematics for arm positioning
- [ ] Implement gesture recognition from PS2 controller
- [ ] Add IMU sensor for balance feedback
- [ ] Bluetooth control via smartphone app
- [ ] Voice command integration
- [ ] Add gripper control for hands
- [ ] Integrate with ROS for advanced control

## References

- **Arduino Servo Library**: https://www.arduino.cc/reference/en/libraries/servo/
- **PS2X Library**: Based on Bill Porter's PS2X library
- **Mecanum Wheels**: https://en.wikipedia.org/wiki/Mecanum_wheel

## License

This project is open source. Feel free to modify and adapt for your own robot projects.

## Author

Reza Shojaghias  
Date: February 2026  
Platform: Arduino Mega 2560

---

**Note**: Always test servos individually before running full sequences. Ensure adequate power supply to prevent brownouts and erratic behavior.
