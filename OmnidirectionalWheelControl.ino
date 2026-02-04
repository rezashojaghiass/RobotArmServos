#include <PS2X_lib.h>

#define PS2_DAT  9    
#define PS2_CMD  10   
#define PS2_SEL  13   
#define PS2_CLK  44   

PS2X ps2x;
int error = 0;

// Motor Pins
#define PWMA 11
#define DIRA1 34
#define DIRA2 35

#define PWMB 7
#define DIRB1 36
#define DIRB2 37

#define PWMC 6
#define DIRC1 43
#define DIRC2 42

#define PWMD 4
#define DIRD1 A5
#define DIRD2 A4

void setMotor(int pwm, int dir1, int dir2, int speed) {
    if (speed > 0) { // Forward
        digitalWrite(dir1, HIGH);
        digitalWrite(dir2, LOW);
    } else if (speed < 0) { // Backward
        digitalWrite(dir1, LOW);
        digitalWrite(dir2, HIGH);
    } else { // Stop
        digitalWrite(dir1, LOW);
        digitalWrite(dir2, LOW);
    }
    analogWrite(pwm, abs(speed));
}

void setup() {
    Serial.begin(9600);
    delay(1000);

    pinMode(PWMA, OUTPUT); pinMode(DIRA1, OUTPUT); pinMode(DIRA2, OUTPUT);
    pinMode(PWMB, OUTPUT); pinMode(DIRB1, OUTPUT); pinMode(DIRB2, OUTPUT);
    pinMode(PWMC, OUTPUT); pinMode(DIRC1, OUTPUT); pinMode(DIRC2, OUTPUT);
    pinMode(PWMD, OUTPUT); pinMode(DIRD1, OUTPUT); pinMode(DIRD2, OUTPUT);

    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);
    if (error == 0) {
        Serial.println("✅ PS2 Controller connected successfully!");
    } else {
        Serial.println("❌ PS2 Controller connection failed! Check wiring.");
    }
}

void loop() {
    if (error) return;  

    ps2x.read_gamepad(false, 0);
    
    int LX = ps2x.Analog(PSS_LX) - 127;  // Left/Right strafing
    int LY = ps2x.Analog(PSS_LY) - 127;  // Forward/Backward
    int RX = ps2x.Analog(PSS_RX) - 127;  // Rotation

    if (LY < -20) {  // **Move Forward**
        setMotor(PWMA, DIRA1, DIRA2, 150);  // A Forward
        setMotor(PWMB, DIRB1, DIRB2, 150);  // B Forward
        setMotor(PWMC, DIRC1, DIRC2, 150);  // C Forward
        setMotor(PWMD, DIRD1, DIRD2, 150);  // D Forward
        Serial.println("⬆️ Moving Forward");
    } 
    else if (LY > 20) {  // **Move Backward**
        setMotor(PWMA, DIRA1, DIRA2, -150);  // A Backward
        setMotor(PWMB, DIRB1, DIRB2, -150);  // B Backward
        setMotor(PWMC, DIRC1, DIRC2, -150);  // C Backward
        setMotor(PWMD, DIRD1, DIRD2, -150);  // D Backward
        Serial.println("⬇️ Moving Backward");
    }
    else if (LX < -20) {  // **Strafe Left**
        setMotor(PWMA, DIRA1, DIRA2, -150);  // A Backward
        setMotor(PWMB, DIRB1, DIRB2, 150);   // B Forward
        setMotor(PWMC, DIRC1, DIRC2, -150);  // C Backward
        setMotor(PWMD, DIRD1, DIRD2, 150);   // D Forward
        Serial.println("⬅️ Strafing Left");
    } 
    else if (LX > 20) {  // **Strafe Right**
        setMotor(PWMA, DIRA1, DIRA2, 150);   // A Forward
        setMotor(PWMB, DIRB1, DIRB2, -150);  // B Backward
        setMotor(PWMC, DIRC1, DIRC2, 150);   // C Forward
        setMotor(PWMD, DIRD1, DIRD2, -150);  // D Backward
        Serial.println("➡️ Strafing Right");
    }
    else if (RX < -20) {  // **Rotate Left**
        setMotor(PWMA, DIRA1, DIRA2, -150);  // A Backward
        setMotor(PWMB, DIRB1, DIRB2, 150);   // B Forward
        setMotor(PWMC, DIRC1, DIRC2, 150);   // C Forward
        setMotor(PWMD, DIRD1, DIRD2, -150);  // D Backward
        Serial.println("↩️ Rotating Left");
    }
    else if (RX > 20) {  // **Rotate Right**
        setMotor(PWMA, DIRA1, DIRA2, 150);   // A Forward
        setMotor(PWMB, DIRB1, DIRB2, -150);  // B Backward
        setMotor(PWMC, DIRC1, DIRC2, -150);  // C Backward
        setMotor(PWMD, DIRD1, DIRD2, 150);   // D Forward
        Serial.println("↪️ Rotating Right");
    } 
    else {
        setMotor(PWMA, DIRA1, DIRA2, 0);
        setMotor(PWMB, DIRB1, DIRB2, 0);
        setMotor(PWMC, DIRC1, DIRC2, 0);
        setMotor(PWMD, DIRD1, DIRD2, 0);
        Serial.println("🛑 Stopped");
    }

    delay(100);
}
