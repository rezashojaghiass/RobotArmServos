#include <Servo.h>
const int REPEAT_COUNT = 1;

// ===== EXTRA SHOULDER DOWN CORRECTION (runs ONCE at the end) =====
const unsigned long EXTRA_DOWN_TOTAL_MS = 0;   // tune this like 700 ms

// ===== Right hand pins =====
#define THUMB_PIN   2
#define INDEX_PIN   3
#define MIDDLE_PIN  4
#define RING_PIN    5
#define PINKY_PIN   6

// ===== Left hand pins =====
#define LTHUMB_PIN   7
#define LINDEX_PIN   8
#define LMIDDLE_PIN  9
#define LRING_PIN    10
#define LPINKY_PIN   11

// ===== LEFT SHOULDER (BIG SERVO) =====
#define LSHOULDER_PIN 14   // signal
#define LSHOULDER_PWR 30   // power enable (HIGH = ON)

Servo sThumb, sIndex, sMiddle, sRing, sPinky;
Servo sLThumb, sLIndex, sLMiddle, sLRing, sLPinky;
Servo sLShoulder;

// === Calibrated endpoints (RIGHT HAND) ===
const int THUMB_OPEN  = 1360;
const int THUMB_CLOSE = 2300;

const int INDEX_OPEN  = 2000;
const int INDEX_CLOSE = 700;

const int MIDDLE_OPEN  = 2000;
const int MIDDLE_CLOSE = 700;

const int RING_OPEN  = 2000;
const int RING_CLOSE = 700;

const int PINKY_OPEN  = 2000;
const int PINKY_CLOSE = 700;

// === LEFT HAND (calibrated & mirrored) ===
const int LTHUMB_OPEN  = 1930;
const int LTHUMB_CLOSE = 740;

// mirrored vs right
const int LINDEX_OPEN   = INDEX_CLOSE;
const int LINDEX_CLOSE  = INDEX_OPEN;

const int LMIDDLE_OPEN  = MIDDLE_CLOSE;
const int LMIDDLE_CLOSE = MIDDLE_OPEN;

const int LRING_OPEN    = RING_CLOSE;
const int LRING_CLOSE   = RING_OPEN;

const int LPINKY_OPEN   = PINKY_CLOSE;
const int LPINKY_CLOSE  = PINKY_OPEN;

// === LEFT SHOULDER pulses (your known good settings) ===
const int LSH_NEUTRAL = 1460;  // stop
const int LSH_UP      = 1900;  // move up
const int LSH_DOWN    = 1300;  // move down

// === Speed control ===
const float SPEED = 1.8;

// Timing (scaled)
const unsigned long PHASE_DELAY = (unsigned long)(500  / SPEED);
const unsigned long HALF_MOVE   = (unsigned long)(1600 / SPEED);
const unsigned long UPDATE_MS   = (unsigned long)(20   / SPEED);

// Strong homing
const unsigned long PREOPEN_MS  = 0;

int repeatsDone = 0;

const unsigned long END_BUFFER  = 80;
const unsigned long BETWEEN_MS  = 20;

unsigned long t0;
unsigned long lastUpdate = 0;
bool done = false;

// ===== Helpers =====
int lerpInt(int a, int b, float t) {
  return a + (int)((b - a) * t);
}

int wavePos(int openUs, int closeUs, long dtMs) {
  if (dtMs <= 0) return openUs;

  if ((unsigned long)dtMs < HALF_MOVE) {
    float t = (float)dtMs / (float)HALF_MOVE;
    return lerpInt(openUs, closeUs, t);
  }

  dtMs -= HALF_MOVE;
  if ((unsigned long)dtMs < HALF_MOVE) {
    float t = (float)dtMs / (float)HALF_MOVE;
    return lerpInt(closeUs, openUs, t);
  }

  return openUs;
}

void quietPin(uint8_t pin, Servo &sv) {
  sv.detach();
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

// ===== LEFT SHOULDER safe helpers =====
void shoulderSignalOff() {
  sLShoulder.detach();
  pinMode(LSHOULDER_PIN, OUTPUT);
  digitalWrite(LSHOULDER_PIN, LOW);
}

void shoulderNeutral() {
  sLShoulder.attach(LSHOULDER_PIN);
  sLShoulder.writeMicroseconds(LSH_NEUTRAL);
  delay(200);
}

void shoulderPowerOn() {
  pinMode(LSHOULDER_PWR, OUTPUT);
  digitalWrite(LSHOULDER_PWR, HIGH);
  delay(250);
}

void shoulderPowerOff() {
  digitalWrite(LSHOULDER_PWR, LOW);
}

// ===== one-time extra down correction (continuous) =====
void shoulderExtraDownCorrection() {
  sLShoulder.writeMicroseconds(LSH_DOWN);
  delay(EXTRA_DOWN_TOTAL_MS);
  sLShoulder.writeMicroseconds(LSH_NEUTRAL);
  delay(200);
}

// ===== Fingers attach/open/home =====
void attachAll() {
  // Right hand
  sThumb.attach(THUMB_PIN);
  sIndex.attach(INDEX_PIN);
  sMiddle.attach(MIDDLE_PIN);
  sRing.attach(RING_PIN);
  sPinky.attach(PINKY_PIN);

  // Left hand
  sLThumb.attach(LTHUMB_PIN);
  sLIndex.attach(LINDEX_PIN);
  sLMiddle.attach(LMIDDLE_PIN);
  sLRing.attach(LRING_PIN);
  sLPinky.attach(LPINKY_PIN);
}

void openAll() {
  // Right hand
  sThumb.writeMicroseconds(THUMB_OPEN);
  sIndex.writeMicroseconds(INDEX_OPEN);
  sMiddle.writeMicroseconds(MIDDLE_OPEN);
  sRing.writeMicroseconds(RING_OPEN);
  sPinky.writeMicroseconds(PINKY_OPEN);

  // Left hand
  sLThumb.writeMicroseconds(LTHUMB_OPEN);
  sLIndex.writeMicroseconds(LINDEX_OPEN);
  sLMiddle.writeMicroseconds(LMIDDLE_OPEN);
  sLRing.writeMicroseconds(LRING_OPEN);
  sLPinky.writeMicroseconds(LPINKY_OPEN);
}

// ✅ NEW: final open that overrides ONLY left thumb
// If your left thumb closes at the end, it likely needs the opposite endpoint here.
void openAllFinal() {
  // Right hand
  sThumb.writeMicroseconds(THUMB_OPEN);
  sIndex.writeMicroseconds(INDEX_OPEN);
  sMiddle.writeMicroseconds(MIDDLE_OPEN);
  sRing.writeMicroseconds(RING_OPEN);
  sPinky.writeMicroseconds(PINKY_OPEN);

  // Left hand (override thumb only)
  sLThumb.writeMicroseconds(LTHUMB_OPEN); // ✅ swap endpoint for FINAL hold
  // If this makes it worse, change to: sLThumb.writeMicroseconds(LTHUMB_OPEN);

  sLIndex.writeMicroseconds(LINDEX_OPEN);
  sLMiddle.writeMicroseconds(LMIDDLE_OPEN);
  sLRing.writeMicroseconds(LRING_OPEN);
  sLPinky.writeMicroseconds(LPINKY_OPEN);
}

// ✅ NEW: guarantee all fingers end OPEN (uses openAllFinal)
void forceOpenAllFinal(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    openAllFinal();
    delay(20);
  }
}

// Actively drive OPEN before wave
void homeAll(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    openAll();
    delay(20);
  }
}

void setup() {
  attachAll();

  // Fingers: force full OPEN before wave
  homeAll(PREOPEN_MS);

  // Shoulder: SAFE sequence (neutral before power)
  pinMode(LSHOULDER_PWR, OUTPUT);
  digitalWrite(LSHOULDER_PWR, LOW);
  shoulderSignalOff();     // quiet

  shoulderNeutral();       // attach + neutral first
  shoulderPowerOn();       // then power ON

  t0 = millis();
}

void loop() {
  if (done) return;

  unsigned long now = millis();
  if (now - lastUpdate < UPDATE_MS) return;
  lastUpdate = now;

  // Right hand timing
  long dt0 = (long)(now - (t0 + 0 * PHASE_DELAY)); // R thumb
  long dt1 = (long)(now - (t0 + 1 * PHASE_DELAY)); // R index
  long dt2 = (long)(now - (t0 + 2 * PHASE_DELAY)); // R middle
  long dt3 = (long)(now - (t0 + 3 * PHASE_DELAY)); // R ring
  long dt4 = (long)(now - (t0 + 4 * PHASE_DELAY)); // R pinky

  // Left hand timing
  long dt5 = (long)(now - (t0 + 5 * PHASE_DELAY)); // L thumb
  long dt6 = (long)(now - (t0 + 6 * PHASE_DELAY)); // L index
  long dt7 = (long)(now - (t0 + 7 * PHASE_DELAY)); // L middle
  long dt8 = (long)(now - (t0 + 8 * PHASE_DELAY)); // L ring
  long dt9 = (long)(now - (t0 + 9 * PHASE_DELAY)); // L pinky

  // Fingers wave
  sThumb.writeMicroseconds(  wavePos(THUMB_OPEN,  THUMB_CLOSE,  dt0) );
  sIndex.writeMicroseconds(  wavePos(INDEX_OPEN,  INDEX_CLOSE,  dt1) );
  sMiddle.writeMicroseconds( wavePos(MIDDLE_OPEN, MIDDLE_CLOSE, dt2) );
  sRing.writeMicroseconds(   wavePos(RING_OPEN,   RING_CLOSE,   dt3) );
  sPinky.writeMicroseconds(  wavePos(PINKY_OPEN,  PINKY_CLOSE,  dt4) );

  sLThumb.writeMicroseconds(  wavePos(LTHUMB_OPEN,  LTHUMB_CLOSE,  dt5) );
  sLIndex.writeMicroseconds(  wavePos(LINDEX_OPEN,  LINDEX_CLOSE,  dt6) );
  sLMiddle.writeMicroseconds( wavePos(LMIDDLE_OPEN, LMIDDLE_CLOSE, dt7) );
  sLRing.writeMicroseconds(   wavePos(LRING_OPEN,   LRING_CLOSE,   dt8) );
  sLPinky.writeMicroseconds(  wavePos(LPINKY_OPEN,  LPINKY_CLOSE,  dt9) );

  // Round duration (your existing logic)
  const unsigned long ROUND_TOTAL = 9 * PHASE_DELAY + 1 * HALF_MOVE + END_BUFFER;
  unsigned long dtRound = now - t0;

  // ===== Shoulder motion during the SAME round =====
  const unsigned long ARM_UP_MS      = (unsigned long)(ROUND_TOTAL * 0.45f);
  const unsigned long ARM_NEUTRAL_MS = 160;
  unsigned long ARM_DOWN_MS = 0;
  if (ROUND_TOTAL > (ARM_UP_MS + ARM_NEUTRAL_MS)) {
    ARM_DOWN_MS = ROUND_TOTAL - ARM_UP_MS - ARM_NEUTRAL_MS;
  }

  if (dtRound < ARM_UP_MS) {
    sLShoulder.writeMicroseconds(LSH_UP);
  } else if (dtRound < ARM_UP_MS + ARM_NEUTRAL_MS) {
    sLShoulder.writeMicroseconds(LSH_NEUTRAL);
  } else if (dtRound < ARM_UP_MS + ARM_NEUTRAL_MS + ARM_DOWN_MS) {
    sLShoulder.writeMicroseconds(LSH_DOWN);
  } else {
    sLShoulder.writeMicroseconds(LSH_NEUTRAL);
  }

  // End round
  if (dtRound > ROUND_TOTAL) {
    repeatsDone++;

    if (repeatsDone < REPEAT_COUNT) {
      openAll();
      sLShoulder.writeMicroseconds(LSH_NEUTRAL);
      delay(BETWEEN_MS);
      t0 = millis();
    } else {
      // Final stop
      openAll();
      sLShoulder.writeMicroseconds(LSH_NEUTRAL);

      // ✅ RE-RUN INITIALIZATION (same as startup)
      homeAll(PREOPEN_MS);   // uses LTHUMB_OPEN = 1930, proven-good state

      // ✅ NEW: guarantee final open WITHOUT closing left thumb
      forceOpenAllFinal(800);

      // Quiet fingers BEFORE shoulder correction
      quietPin(THUMB_PIN,  sThumb);
      quietPin(INDEX_PIN,  sIndex);
      quietPin(MIDDLE_PIN, sMiddle);
      quietPin(RING_PIN,   sRing);
      quietPin(PINKY_PIN,  sPinky);

      quietPin(LTHUMB_PIN,  sLThumb);
      quietPin(LINDEX_PIN,  sLIndex);
      quietPin(LMIDDLE_PIN, sLMiddle);
      quietPin(LRING_PIN,   sLRing);
      quietPin(LPINKY_PIN,  sLPinky);

      delay(50);

      // Shoulder correction (shoulder only)
      shoulderExtraDownCorrection();

      // Shoulder quiet + power off
      shoulderSignalOff();
      delay(50);
      shoulderPowerOff();

      done = true;
    }
  }
}
