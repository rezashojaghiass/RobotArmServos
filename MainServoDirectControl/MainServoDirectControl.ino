#include <Servo.h>

// ============================================================
// Direct single-servo control for 6 main arm servos
// Set ENABLE_* = true only for the servo you want to move.
// For each enabled servo:
//   - set *_FORWARD or *_BACKWARD (only one should be true)
//   - set *_MOVE_MS duration (e.g. 1000 for 1 second)
// ============================================================

const int SERVO_MIN_US = 600;
const int SERVO_MAX_US = 2400;

// ================== PIN MAP ==================
// LEFT ARM
#define L_SH1_PIN 17
#define L_SH1_PWR 33

#define L_ELB_PIN 24
#define L_ELB_PWR 34

#define L_SH2_PIN 25
#define L_SH2_PWR 35

// RIGHT ARM
#define R_SH1_PIN 14
#define R_SH1_PWR 30

#define R_SH2_PIN 15
#define R_SH2_PWR 31

#define R_ELB_PIN 16
#define R_ELB_PWR 32

// ================== CAL VALUES ==================
// Continuous servos (shoulders)
const int LSH1_NEUTRAL = 1370;
const int LSH1_FORWARD = 1700;
const int LSH1_BACKWARD = 700;

const int LSH2_NEUTRAL = 1500;
const int LSH2_FORWARD = 2300;
const int LSH2_BACKWARD = 700;

const int RSH1_NEUTRAL = 1460;
const int RSH1_FORWARD = 1900;
const int RSH1_BACKWARD = 1300;

const int RSH2_NEUTRAL = 1460;
const int RSH2_FORWARD = 2300;
const int RSH2_BACKWARD = 600;

// Positional servos (elbows)
const int LELB_FORWARD = 1500;
const int LELB_BACKWARD = 880;

const int RELB_FORWARD = 1800;
const int RELB_BACKWARD = 1200;

// ================== USER FLAGS ==================
// Enable only what you want to test
const bool ENABLE_L_SH1 = false;
const bool ENABLE_L_ELB = false;
const bool ENABLE_L_SH2 = false;
const bool ENABLE_R_SH1 = true;
const bool ENABLE_R_ELB = false;
const bool ENABLE_R_SH2 = false;

// Direction flags per servo (set only one true)
const bool L_SH1_FORWARD_FLAG = false;
const bool L_SH1_BACKWARD_FLAG = true;

const bool L_ELB_FORWARD_FLAG = false;
const bool L_ELB_BACKWARD_FLAG = true;

const bool L_SH2_FORWARD_FLAG = true;
const bool L_SH2_BACKWARD_FLAG = false;

const bool R_SH1_FORWARD_FLAG = false;
const bool R_SH1_BACKWARD_FLAG = true;

const bool R_ELB_FORWARD_FLAG = false;
const bool R_ELB_BACKWARD_FLAG = true;

const bool R_SH2_FORWARD_FLAG = true;
const bool R_SH2_BACKWARD_FLAG = false;

// Move time per servo in milliseconds (200 = 0.2 second)
const unsigned long L_SH1_MOVE_MS = 300;
const unsigned long L_ELB_MOVE_MS = 200;
const unsigned long L_SH2_MOVE_MS = 200;

const unsigned long R_SH1_MOVE_MS = 500;
const unsigned long R_ELB_MOVE_MS = 500;
const unsigned long R_SH2_MOVE_MS = 200;

// ================== SERVO OBJECTS ==================
Servo sLSh1, sLElb, sLSh2;
Servo sRSh1, sRElb, sRSh2;

int clampUs(int us) {
  if (us < SERVO_MIN_US) return SERVO_MIN_US;
  if (us > SERVO_MAX_US) return SERVO_MAX_US;
  return us;
}

void powerOn(uint8_t pwrPin) {
  pinMode(pwrPin, OUTPUT);
  digitalWrite(pwrPin, HIGH);
  delay(250);
}

void powerOff(uint8_t pwrPin) {
  digitalWrite(pwrPin, LOW);
}

bool validDirection(bool forwardFlag, bool backwardFlag) {
  // exactly one direction flag must be true
  return (forwardFlag ^ backwardFlag);
}

void moveContinuous(Servo &s, uint8_t pwrPin, int signalPin, int neutralUs, int forwardUs, int backwardUs,
                    bool forwardFlag, bool backwardFlag, unsigned long moveMs, const char *name) {
  if (!validDirection(forwardFlag, backwardFlag)) {
    Serial.print("Skip ");
    Serial.print(name);
    Serial.println(" (set exactly one direction flag true)");
    return;
  }

  powerOn(pwrPin);
  s.attach(signalPin);
  s.writeMicroseconds(clampUs(neutralUs));
  delay(200);

  int target = forwardFlag ? forwardUs : backwardUs;
  s.writeMicroseconds(clampUs(target));
  delay(moveMs);

  // Stop continuous servo at neutral
  s.writeMicroseconds(clampUs(neutralUs));
  delay(100);

  s.detach();
  powerOff(pwrPin);
}

void movePositional(Servo &s, uint8_t pwrPin, int signalPin, int forwardUs, int backwardUs,
                    bool forwardFlag, bool backwardFlag, unsigned long moveMs, const char *name) {
  if (!validDirection(forwardFlag, backwardFlag)) {
    Serial.print("Skip ");
    Serial.print(name);
    Serial.println(" (set exactly one direction flag true)");
    return;
  }

  powerOn(pwrPin);
  s.attach(signalPin);
  delay(200);

  int target = forwardFlag ? forwardUs : backwardUs;
  s.writeMicroseconds(clampUs(target));
  delay(moveMs);

  s.detach();
  powerOff(pwrPin);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("MainServoDirectControl start");

  // safe default: power rails low
  pinMode(L_SH1_PWR, OUTPUT); digitalWrite(L_SH1_PWR, LOW);
  pinMode(L_ELB_PWR, OUTPUT); digitalWrite(L_ELB_PWR, LOW);
  pinMode(L_SH2_PWR, OUTPUT); digitalWrite(L_SH2_PWR, LOW);

  pinMode(R_SH1_PWR, OUTPUT); digitalWrite(R_SH1_PWR, LOW);
  pinMode(R_ELB_PWR, OUTPUT); digitalWrite(R_ELB_PWR, LOW);
  pinMode(R_SH2_PWR, OUTPUT); digitalWrite(R_SH2_PWR, LOW);

  if (ENABLE_L_SH1) {
    moveContinuous(sLSh1, L_SH1_PWR, L_SH1_PIN,
                   LSH1_NEUTRAL, LSH1_FORWARD, LSH1_BACKWARD,
                   L_SH1_FORWARD_FLAG, L_SH1_BACKWARD_FLAG,
                   L_SH1_MOVE_MS, "L_SH1");
  }

  if (ENABLE_L_ELB) {
    movePositional(sLElb, L_ELB_PWR, L_ELB_PIN,
                   LELB_FORWARD, LELB_BACKWARD,
                   L_ELB_FORWARD_FLAG, L_ELB_BACKWARD_FLAG,
                   L_ELB_MOVE_MS, "L_ELB");
  }

  if (ENABLE_L_SH2) {
    moveContinuous(sLSh2, L_SH2_PWR, L_SH2_PIN,
                   LSH2_NEUTRAL, LSH2_FORWARD, LSH2_BACKWARD,
                   L_SH2_FORWARD_FLAG, L_SH2_BACKWARD_FLAG,
                   L_SH2_MOVE_MS, "L_SH2");
  }

  if (ENABLE_R_SH1) {
    moveContinuous(sRSh1, R_SH1_PWR, R_SH1_PIN,
                   RSH1_NEUTRAL, RSH1_FORWARD, RSH1_BACKWARD,
                   R_SH1_FORWARD_FLAG, R_SH1_BACKWARD_FLAG,
                   R_SH1_MOVE_MS, "R_SH1");
  }

  if (ENABLE_R_ELB) {
    movePositional(sRElb, R_ELB_PWR, R_ELB_PIN,
                   RELB_FORWARD, RELB_BACKWARD,
                   R_ELB_FORWARD_FLAG, R_ELB_BACKWARD_FLAG,
                   R_ELB_MOVE_MS, "R_ELB");
  }

  if (ENABLE_R_SH2) {
    moveContinuous(sRSh2, R_SH2_PWR, R_SH2_PIN,
                   RSH2_NEUTRAL, RSH2_FORWARD, RSH2_BACKWARD,
                   R_SH2_FORWARD_FLAG, R_SH2_BACKWARD_FLAG,
                   R_SH2_MOVE_MS, "R_SH2");
  }

  Serial.println("Done. No repeating loop.");
}

void loop() {
  // One-shot behavior: everything is done in setup().
}
