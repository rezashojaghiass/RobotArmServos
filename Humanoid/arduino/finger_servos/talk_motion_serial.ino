// ============================================================
// TALK MOTION SERIAL - Arduino #1 (ALL FINGERS + ELBOWS)
// ============================================================

#include <Servo.h>
#include <math.h>

// Right hand
#define THUMB_PIN   2
#define INDEX_PIN   3
#define MIDDLE_PIN  4
#define RING_PIN    5
#define PINKY_PIN   6

// Left hand
#define LTHUMB_PIN   7
#define LINDEX_PIN   8
#define LMIDDLE_PIN  9
#define LRING_PIN    10
#define LPINKY_PIN   11

// Shoulders
#define L_SH1_PIN 17
#define R_SH1_PIN 14
#define R_SH2_PIN 15

// Elbows
#define L_ELB_PIN 24
#define L_SH2_PIN 25
#define R_ELB_PIN 16

// Power rails
#define L_SH1_PWR 33
#define L_SH2_PWR 35
#define R_SH1_PWR 30
#define R_SH2_PWR 31
#define L_ELB_PWR 34
#define R_ELB_PWR 32

const int SERVO_MIN_US = 600;
const int SERVO_MAX_US = 2400;

const float FINGER_CLOSE_AMOUNT = 0.30f;
const unsigned long FINGER_CYCLE_MS = 1200;

const int L_SH1_NEUTRAL = 1370;
const int L_SH1_UP = 1700;
const int L_SH1_DOWN = 700;

const int L_SH2_NEUTRAL = 1500;
const int L_SH2_UP = 2300;
const int L_SH2_DOWN = 700;

const int R_SH1_NEUTRAL = 1460;
const int R_SH1_UP = 1900;
const int R_SH1_DOWN = 1300;

const int R_SH2_NEUTRAL = 1460;
const int R_SH2_UP = 2300;
const int R_SH2_DOWN = 600;

const int L_ELB_OPEN = 1500;
const int L_ELB_BEND = 1800;
const int R_ELB_OPEN = 1200;
const int R_ELB_BEND = 1800;
const float ELBOW_BEND_AMOUNT = 0.35f;
const unsigned long ELBOW_CYCLE_MS = 700;
const int ELBOW_STEP_US = 120;
const unsigned long SHOULDER_STEP_MS = 220;
const unsigned long ARM_SETTLE_MS = 120;

const unsigned long MIN_TALK_ACTIVE_MS = 1200;
const unsigned long UPDATE_MS = 20;

// Finger calibration
const int THUMB_OPEN  = 1500, THUMB_CLOSE  = 2300;
const int INDEX_OPEN  = 2000, INDEX_CLOSE  = 700;
const int MIDDLE_OPEN = 2000, MIDDLE_CLOSE = 700;
const int RING_OPEN   = 2000, RING_CLOSE   = 700;
const int PINKY_OPEN  = 2000, PINKY_CLOSE  = 700;

const int LTHUMB_OPEN  = 1500, LTHUMB_CLOSE  = 740;
const int LINDEX_OPEN  = INDEX_CLOSE,  LINDEX_CLOSE  = INDEX_OPEN;
const int LMIDDLE_OPEN = MIDDLE_CLOSE, LMIDDLE_CLOSE = MIDDLE_OPEN;
const int LRING_OPEN   = RING_CLOSE,   LRING_CLOSE   = RING_OPEN;
const int LPINKY_OPEN  = PINKY_CLOSE,  LPINKY_CLOSE  = PINKY_OPEN;

Servo sThumb, sIndex, sMiddle, sRing, sPinky;
Servo sLThumb, sLIndex, sLMiddle, sLRing, sLPinky;
Servo sLSh1, sLSh2;
Servo sLElb, sRElb;
Servo sRSh1, sRSh2;

bool talking = false;
bool pendingStop = false;
unsigned long lastUpdateMs = 0;
unsigned long talkStartedMs = 0;
int currentLElbUs = (L_ELB_OPEN + L_ELB_BEND) / 2;
int currentRElbUs = (R_ELB_OPEN + R_ELB_BEND) / 2;

int clampUs(int us) {
  if (us < SERVO_MIN_US) return SERVO_MIN_US;
  if (us > SERVO_MAX_US) return SERVO_MAX_US;
  return us;
}

int lerpInt(int a, int b, float t) {
  return a + (int)((b - a) * t);
}

bool contains(const String &line, const char *token) {
  return line.indexOf(token) >= 0;
}

String compactJson(String s) {
  s.replace(" ", "");
  s.replace("\t", "");
  return s;
}

void attachAllIfNeeded() {
  if (!sThumb.attached()) sThumb.attach(THUMB_PIN);
  if (!sIndex.attached()) sIndex.attach(INDEX_PIN);
  if (!sMiddle.attached()) sMiddle.attach(MIDDLE_PIN);
  if (!sRing.attached()) sRing.attach(RING_PIN);
  if (!sPinky.attached()) sPinky.attach(PINKY_PIN);

  if (!sLThumb.attached()) sLThumb.attach(LTHUMB_PIN);
  if (!sLIndex.attached()) sLIndex.attach(LINDEX_PIN);
  if (!sLMiddle.attached()) sLMiddle.attach(LMIDDLE_PIN);
  if (!sLRing.attached()) sLRing.attach(LRING_PIN);
  if (!sLPinky.attached()) sLPinky.attach(LPINKY_PIN);

  if (!sLElb.attached()) sLElb.attach(L_ELB_PIN);
  if (!sRElb.attached()) sRElb.attach(R_ELB_PIN);
}

void detachAllServos() {
  if (sThumb.attached()) sThumb.detach();
  if (sIndex.attached()) sIndex.detach();
  if (sMiddle.attached()) sMiddle.detach();
  if (sRing.attached()) sRing.detach();
  if (sPinky.attached()) sPinky.detach();

  if (sLThumb.attached()) sLThumb.detach();
  if (sLIndex.attached()) sLIndex.detach();
  if (sLMiddle.attached()) sLMiddle.detach();
  if (sLRing.attached()) sLRing.detach();
  if (sLPinky.attached()) sLPinky.detach();

  if (sLSh1.attached()) sLSh1.detach();
  if (sLSh2.attached()) sLSh2.detach();
  if (sLElb.attached()) sLElb.detach();
  if (sRSh1.attached()) sRSh1.detach();
  if (sRSh2.attached()) sRSh2.detach();
  if (sRElb.attached()) sRElb.detach();
}

void powerOnRail(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delay(ARM_SETTLE_MS);
}

void powerOffRail(uint8_t pin) {
  digitalWrite(pin, LOW);
}

void attachShoulderIfNeeded(Servo &servo, uint8_t signalPin, int neutralPulse) {
  if (!servo.attached()) {
    servo.attach(signalPin);
  }
  servo.writeMicroseconds(clampUs(neutralPulse));
  delay(80);
}

void attachElbowIfNeeded(Servo &servo, uint8_t signalPin, int currentPulse) {
  if (!servo.attached()) {
    servo.attach(signalPin);
  }
  servo.writeMicroseconds(clampUs(currentPulse));
  delay(80);
}

void applyShoulderStep(Servo &servo, uint8_t powerPin, uint8_t signalPin, int neutralPulse, int stepPulse) {
  powerOnRail(powerPin);
  attachShoulderIfNeeded(servo, signalPin, neutralPulse);
  servo.writeMicroseconds(clampUs(stepPulse));
  delay(SHOULDER_STEP_MS);
  servo.writeMicroseconds(clampUs(neutralPulse));
  delay(80);
  servo.detach();
  powerOffRail(powerPin);
}

void applyElbowStep(Servo &servo, uint8_t powerPin, uint8_t signalPin, int &currentPulse, int openPulse, int bendPulse, bool moveUp) {
  int minPulse = openPulse < bendPulse ? openPulse : bendPulse;
  int maxPulse = openPulse > bendPulse ? openPulse : bendPulse;
  int targetPulse = currentPulse + (moveUp ? ELBOW_STEP_US : -ELBOW_STEP_US);
  if (targetPulse < minPulse) targetPulse = minPulse;
  if (targetPulse > maxPulse) targetPulse = maxPulse;

  powerOnRail(powerPin);
  attachElbowIfNeeded(servo, signalPin, currentPulse);
  currentPulse = targetPulse;
  servo.writeMicroseconds(clampUs(currentPulse));
  delay(180);
  servo.detach();
  powerOffRail(powerPin);
}

void writeOpenPose() {
  sThumb.writeMicroseconds(clampUs(THUMB_OPEN));
  sIndex.writeMicroseconds(clampUs(INDEX_OPEN));
  sMiddle.writeMicroseconds(clampUs(MIDDLE_OPEN));
  sRing.writeMicroseconds(clampUs(RING_OPEN));
  sPinky.writeMicroseconds(clampUs(PINKY_OPEN));

  sLThumb.writeMicroseconds(clampUs(LTHUMB_OPEN));
  sLIndex.writeMicroseconds(clampUs(LINDEX_OPEN));
  sLMiddle.writeMicroseconds(clampUs(LMIDDLE_OPEN));
  sLRing.writeMicroseconds(clampUs(LRING_OPEN));
  sLPinky.writeMicroseconds(clampUs(LPINKY_OPEN));

  sLElb.writeMicroseconds(clampUs(L_ELB_OPEN));
  sRElb.writeMicroseconds(clampUs(R_ELB_OPEN));
}

void writeTalkMotion(unsigned long now) {
  float fingerPhase = ((float)(now - talkStartedMs) / (float)FINGER_CYCLE_MS);
  fingerPhase = fingerPhase - floor(fingerPhase);
  float fingerSmooth = 0.5f - 0.5f * cosf(fingerPhase * 6.2831853f);
  float ft = fingerSmooth * FINGER_CLOSE_AMOUNT;

  sThumb.writeMicroseconds(clampUs(lerpInt(THUMB_OPEN, THUMB_CLOSE, ft)));
  sIndex.writeMicroseconds(clampUs(lerpInt(INDEX_OPEN, INDEX_CLOSE, ft)));
  sMiddle.writeMicroseconds(clampUs(lerpInt(MIDDLE_OPEN, MIDDLE_CLOSE, ft)));
  sRing.writeMicroseconds(clampUs(lerpInt(RING_OPEN, RING_CLOSE, ft)));
  sPinky.writeMicroseconds(clampUs(lerpInt(PINKY_OPEN, PINKY_CLOSE, ft)));

  sLThumb.writeMicroseconds(clampUs(lerpInt(LTHUMB_OPEN, LTHUMB_CLOSE, ft)));
  sLIndex.writeMicroseconds(clampUs(lerpInt(LINDEX_OPEN, LINDEX_CLOSE, ft)));
  sLMiddle.writeMicroseconds(clampUs(lerpInt(LMIDDLE_OPEN, LMIDDLE_CLOSE, ft)));
  sLRing.writeMicroseconds(clampUs(lerpInt(LRING_OPEN, LRING_CLOSE, ft)));
  sLPinky.writeMicroseconds(clampUs(lerpInt(LPINKY_OPEN, LPINKY_CLOSE, ft)));

  float elbowPhase = ((float)(now - talkStartedMs) / (float)ELBOW_CYCLE_MS);
  elbowPhase = elbowPhase - floor(elbowPhase);
  float elbowSmooth = 0.5f - 0.5f * cosf(elbowPhase * 6.2831853f);
  float et = elbowSmooth * ELBOW_BEND_AMOUNT;

  sLElb.writeMicroseconds(clampUs(lerpInt(L_ELB_OPEN, L_ELB_BEND, et)));
  sRElb.writeMicroseconds(clampUs(lerpInt(R_ELB_OPEN, R_ELB_BEND, et)));
}

void startTalkingMotion() {
  if (talking) return;

  digitalWrite(L_ELB_PWR, HIGH);
  digitalWrite(R_ELB_PWR, HIGH);
  delay(100);

  attachAllIfNeeded();
  writeOpenPose();
  delay(120);

  talking = true;
  pendingStop = false;
  talkStartedMs = millis();
  lastUpdateMs = 0;

  Serial.println("ACK TALK_ON");
}

void stopTalkingMotionNow() {
  talking = false;
  pendingStop = false;

  writeOpenPose();
  delay(120);
  detachAllServos();

  digitalWrite(L_ELB_PWR, LOW);
  digitalWrite(R_ELB_PWR, LOW);

  Serial.println("ACK TALK_OFF");
}

void handleArmCalibration(const String &line) {
  int p1 = line.indexOf(':');
  int p2 = line.indexOf(':', p1 + 1);
  int p3 = line.indexOf(':', p2 + 1);
  int p4 = line.indexOf(':', p3 + 1);

  if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0) {
    Serial.println("ERR ARM_CAL PARSE");
    return;
  }

  String side = line.substring(p1 + 1, p2);
  String joint = line.substring(p2 + 1, p3);
  String direction = line.substring(p3 + 1, p4);
  String amount = line.substring(p4 + 1);

  side.trim();
  joint.trim();
  direction.trim();
  amount.trim();

  side.toUpperCase();
  joint.toUpperCase();
  direction.toUpperCase();
  amount.toUpperCase();

  if (talking) {
    stopTalkingMotionNow();
  }

  if (amount != "SMALL") {
    Serial.println("ERR ARM_CAL AMOUNT");
    return;
  }

  bool moveUp = direction == "UP";

  if (side == "LEFT" && joint == "ELBOW") {
    applyElbowStep(sLElb, L_ELB_PWR, L_ELB_PIN, currentLElbUs, L_ELB_OPEN, L_ELB_BEND, moveUp);
    Serial.println("ACK ARM_CAL LEFT ELBOW");
    return;
  }

  if (side == "RIGHT" && joint == "ELBOW") {
    applyElbowStep(sRElb, R_ELB_PWR, R_ELB_PIN, currentRElbUs, R_ELB_OPEN, R_ELB_BEND, moveUp);
    Serial.println("ACK ARM_CAL RIGHT ELBOW");
    return;
  }

  if (side == "LEFT" && joint == "SHOULDER1") {
    applyShoulderStep(sLSh1, L_SH1_PWR, L_SH1_PIN, L_SH1_NEUTRAL, moveUp ? L_SH1_UP : L_SH1_DOWN);
    Serial.println("ACK ARM_CAL LEFT SHOULDER1");
    return;
  }

  if (side == "LEFT" && joint == "SHOULDER2") {
    applyShoulderStep(sLSh2, L_SH2_PWR, L_SH2_PIN, L_SH2_NEUTRAL, moveUp ? L_SH2_UP : L_SH2_DOWN);
    Serial.println("ACK ARM_CAL LEFT SHOULDER2");
    return;
  }

  if (side == "RIGHT" && joint == "SHOULDER1") {
    applyShoulderStep(sRSh1, R_SH1_PWR, R_SH1_PIN, R_SH1_NEUTRAL, moveUp ? R_SH1_UP : R_SH1_DOWN);
    Serial.println("ACK ARM_CAL RIGHT SHOULDER1");
    return;
  }

  if (side == "RIGHT" && joint == "SHOULDER2") {
    applyShoulderStep(sRSh2, R_SH2_PWR, R_SH2_PIN, R_SH2_NEUTRAL, moveUp ? R_SH2_UP : R_SH2_DOWN);
    Serial.println("ACK ARM_CAL RIGHT SHOULDER2");
    return;
  }

  Serial.println("ERR ARM_CAL TARGET");
}

void stopTalkingMotion() {
  if (!talking) return;
  if (millis() - talkStartedMs < MIN_TALK_ACTIVE_MS) {
    pendingStop = true;
    return;
  }
  stopTalkingMotionNow();
}

void handleLine(const String &raw) {
  String line = raw;
  line.trim();
  if (line.length() == 0) return;

  if (line.startsWith("ARM_CAL:")) {
    handleArmCalibration(line);
    return;
  }

  String compact = compactJson(line);

  if (line == "TALK_ON") { startTalkingMotion(); return; }
  if (line == "TALK_OFF") { stopTalkingMotion(); return; }

  if (contains(compact, "\"type\":\"gesture_start\"")) { startTalkingMotion(); return; }
  if (contains(compact, "\"type\":\"gesture_stop\"")) { stopTalkingMotion(); return; }
  if (contains(compact, "\"type\":\"speech_start\"")) { startTalkingMotion(); return; }
  if (contains(compact, "\"type\":\"speech_stop\"")) { stopTalkingMotion(); return; }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(L_SH1_PWR, OUTPUT); digitalWrite(L_SH1_PWR, LOW);
  pinMode(L_SH2_PWR, OUTPUT); digitalWrite(L_SH2_PWR, LOW);
  pinMode(R_SH1_PWR, OUTPUT); digitalWrite(R_SH1_PWR, LOW);
  pinMode(R_SH2_PWR, OUTPUT); digitalWrite(R_SH2_PWR, LOW);

  pinMode(L_ELB_PWR, OUTPUT); digitalWrite(L_ELB_PWR, LOW);
  pinMode(R_ELB_PWR, OUTPUT); digitalWrite(R_ELB_PWR, LOW);

  digitalWrite(L_ELB_PWR, HIGH);
  digitalWrite(R_ELB_PWR, HIGH);
  delay(100);
  attachAllIfNeeded();
  writeOpenPose();
  delay(120);
  detachAllServos();
  digitalWrite(L_ELB_PWR, LOW);
  digitalWrite(R_ELB_PWR, LOW);

  Serial.println("READY TALK_MOTION_SERIAL FINGERS_ELBOWS");
}

void loop() {
  static String line = "";

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      handleLine(line);
      line = "";
    } else if (c != '\r') {
      line += c;
      if (line.length() > 220) line = "";
    }
  }

  if (!talking) return;

  unsigned long now = millis();
  if (now - lastUpdateMs < UPDATE_MS) return;
  lastUpdateMs = now;

  writeTalkMotion(now);

  if (pendingStop && (now - talkStartedMs >= MIN_TALK_ACTIVE_MS)) {
    stopTalkingMotionNow();
  }
}
