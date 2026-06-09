// PlatformIO C++ source files need the Arduino core header explicitly.
#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"


Servo servo1;
Servo servo2;

#define SERVO_PIN1 26
#define SERVO_PIN2 27
//color sensor pin
#define SENSOR_LED_PIN 45
constexpr uint8_t PCA_ADDR = 0x70;
constexpr uint8_t COLOR_SENSOR_COUNT = 2;
const uint16_t colorThreshold = 2000;



// =====================================
// MOTOR PINS (IBT-2)
// =====================================
#define M1_RPWM 4
#define M1_LPWM 5

#define M2_RPWM 6
#define M2_LPWM 7

//Servo pins
#define XSTEP_PIN  35
#define XDIR_PIN   34
#define YSTEP_PIN  37
#define YDIR_PIN   36

// =====================================
// ENCODERS
// =====================================
#define LEFT_ENCODER 18
#define RIGHT_ENCODER 19

//counter Pins
#define leftCounter 22
#define rightCounter 25

volatile long leftEncoderTick = 0;
volatile long rightEncoderTick = 0;

unsigned long lastCountTime = 0;
const unsigned long debounceTime = 300; // ms

// =====================================
// IR THRESHOLD
// =====================================
int threshold = 750;


// =====================================
// SPEEDS
// =====================================
int baseSpeed =80 ;
int turnSpeed = 20;
int encoderTurnSpeed=135;

//color sensitivity gain
// Adafruit_TCS34725 tcs = Adafruit_TCS34725();

// This boosts the sensor's sensitivity to maximum (60x gain) and lets it look at light longer (154ms)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_60X);

// =====================================
// FORWARD IR ARRAY
// =====================================
#define FLMS A0
#define FLS  A4
#define FCS  A5
#define FRS  A6
#define FRMS A7

// =====================================
// BACK IR ARRAY
// =====================================
#define BLMS A8
#define BLS  A12
#define BCS  A13
#define BRS  A14
#define BRMS A15

// =====================================
// ENCODER ISR
// =====================================
void leftEncoderISR() { leftEncoderTick++; }
void rightEncoderISR() { rightEncoderTick++; }

// =====================================
// MOTOR CONTROL (IBT-2)
// =====================================
void setMotor(int leftSpeed, int rightSpeed) {

  // Left Motor
  if (leftSpeed >= 0) {
    analogWrite(M1_RPWM, leftSpeed);
    analogWrite(M1_LPWM, 0);
  } else {
    analogWrite(M1_RPWM, 0);
    analogWrite(M1_LPWM, -leftSpeed);
  }

  // Right Motor
  if (rightSpeed >= 0) {
    analogWrite(M2_RPWM, rightSpeed);
    analogWrite(M2_LPWM, 0);
  } else {
    analogWrite(M2_RPWM, 0);
    analogWrite(M2_LPWM, -rightSpeed);
  }
}

void stopMotors() {
  analogWrite(M1_RPWM, 0);
  analogWrite(M1_LPWM, 0);
  analogWrite(M2_RPWM, 0);
  analogWrite(M2_LPWM, 0);
}

void haltMotors() {
  analogWrite(M1_RPWM, 255);
  analogWrite(M1_LPWM, 255);
  analogWrite(M2_RPWM, 255);
  analogWrite(M2_LPWM, 255);
}
// =====================================
// SENSOR PRINTING
// =====================================
void printForwardSensors() {
  Serial.print(" F:[");
  Serial.print(analogRead(FLMS)); Serial.print(" ");
  Serial.print(analogRead(FLS));  Serial.print(" ");
  Serial.print(analogRead(FCS));  Serial.print(" ");
  Serial.print(analogRead(FRS));  Serial.print(" ");
  Serial.print(analogRead(FRMS));
  Serial.print("]");
}

void printBackwardSensors() {
  Serial.print(" B:[");
  Serial.print(analogRead(BLMS)); Serial.print(" ");
  Serial.print(analogRead(BLS));  Serial.print(" ");
  Serial.print(analogRead(BCS));  Serial.print(" ");
  Serial.print(analogRead(BRS));  Serial.print(" ");
  Serial.print(analogRead(BRMS));
  Serial.print("]");
}

// =====================================
// LINE FOLLOW FORWARD
// =====================================
void lineFollowForward() {

  int L2 = analogRead(FLMS);
  int L1 = analogRead(FLS);
  int C  = analogRead(FCS);
  int R1 = analogRead(FRS);
  int R2 = analogRead(FRMS);

  bool fL2 = (L2 < threshold);
  bool fL1 = (L1 < threshold);
  bool fC  = (C  < threshold);
  bool fR1 = (R1 < threshold);
  bool fR2 = (R2 < threshold);

  if (fC) setMotor(baseSpeed-5, baseSpeed);
  else if (fR1) setMotor(turnSpeed, baseSpeed);
  else if (fR2) setMotor(0, baseSpeed);
  else if (fL1) setMotor(baseSpeed, turnSpeed);
  else if (fL2) setMotor(baseSpeed, 0);
  else stopMotors();
}

// =====================================
// LINE FOLLOW BACKWARD
// =====================================
void lineFollowBackward() {

  int L2 = analogRead(BLMS);
  int L1 = analogRead(BLS);
  int C  = analogRead(BCS);
  int R1 = analogRead(BRS);
  int R2 = analogRead(BRMS);

  bool bL2 = (L2 < threshold);
  bool bL1 = (L1 < threshold);
  bool bC  = (C  < threshold);
  bool bR1 = (R1 < threshold);
  bool bR2 = (R2 < threshold);

  

  if (bC) setMotor(-baseSpeed+5, -baseSpeed);
  else if (bL1) setMotor(-turnSpeed, -baseSpeed);
  else if (bL2) setMotor(0, -baseSpeed);
  else if (bR1) setMotor(-baseSpeed, -turnSpeed);
  else if (bR2) setMotor(-baseSpeed, 0);
  else {

  }
}

//ticks functions
void encoderForward(long ticks) {
  leftEncoderTick = 0;
  rightEncoderTick = 0;

  while (leftEncoderTick < ticks && rightEncoderTick < ticks) {
    lineFollowForward();
  }

  stopMotors();
}

void encoderBackward(long ticks) {
  leftEncoderTick = 0;
  rightEncoderTick = 0;

  while (leftEncoderTick < ticks && rightEncoderTick < ticks) {
    lineFollowBackward();
  }

  stopMotors();
}

void encoderRight(long ticks) {
  leftEncoderTick = 0;
  rightEncoderTick = 0;

  while (leftEncoderTick < ticks && rightEncoderTick < ticks) {
    setMotor(-encoderTurnSpeed, encoderTurnSpeed);
  }

  stopMotors();
}

void encoderLeft(long ticks) {
  leftEncoderTick = 0;
  rightEncoderTick = 0;

  while (leftEncoderTick < ticks && rightEncoderTick < ticks) {
    setMotor(encoderTurnSpeed, -encoderTurnSpeed);
  }

  stopMotors();
}

// =====================================
// NEW FUNCTION: LINE FOLLOW UNTIL COUNTER
// =====================================
void lineFollowUntil(int targetCount) {

  int counter = 0;
  bool prevAllBlack = false;

  while (true) {  // <-- changed: we control exit ourselves

    // Read sensors FIRST before moving
    int c[2];
    c[0] = digitalRead(leftCounter);
    c[1] = digitalRead(rightCounter);

    // Check all-black junction
    bool allBlack =
      (c[0] == HIGH ||
       c[1] == HIGH
      );

    // Rising edge: new junction detected
    if (allBlack && !prevAllBlack && (millis() - lastCountTime > debounceTime)) {
      counter++;
      lastCountTime = millis();

      Serial.print(">>> Junction: ");
      Serial.println(counter);

      // Stop IMMEDIATELY when target reached, before moving again
      if (counter >= targetCount) {
        stopMotors();
        // haltMotors();
        Serial.println(">>> Target reached. Stopping.");
        return;  // <-- exit here, motors already stopped
      }
    }

    prevAllBlack = allBlack;

    // Only follow line if target not yet reached
    lineFollowForward();

 

    // Removed delay(500) -- it was causing sluggish stop response
    delay(20);  // small yield only
  }
}

void B_lineFollowUntil(int targetCount) {

  int counter = 0;
  bool prevAllBlack = false;

  while (true) {  // <-- changed: we control exit ourselves

    // Read sensors FIRST before moving
    int c[2];
    c[0] = digitalRead(leftCounter);
    c[1] = digitalRead(rightCounter);

    // Check all-black junction
    bool allBlack =
      (c[0] == HIGH &&
       c[1] == HIGH
      );

    // Rising edge: new junction detected
    if (allBlack && !prevAllBlack && (millis() - lastCountTime > debounceTime)) {
      counter++;
      lastCountTime = millis();

      Serial.print(">>> Junction: ");
      Serial.println(counter);

      // Stop IMMEDIATELY when target reached, before moving again
      if (counter >= targetCount) {
        stopMotors();
        Serial.println(">>> Target reached. Stopping.");
        return;  // <-- exit here, motors already stopped
      }
    }

    prevAllBlack = allBlack;

    // Only follow line if target not yet reached
    lineFollowBackward();

 

    // Removed delay(500) -- it was causing sluggish stop response
    delay(20);  // small yield only
  }
}

void forward(){
   setMotor(baseSpeed-5, baseSpeed);
}

void stepperRight(){
    digitalWrite(XDIR_PIN, HIGH);
  digitalWrite(YDIR_PIN, HIGH);
   
  //x motor 
    for(int i = 0; i < 200; i++) {

    digitalWrite(XSTEP_PIN, HIGH);
    delayMicroseconds(800);

    digitalWrite(XSTEP_PIN, LOW);
    delayMicroseconds(800);

}
}

void stepperLeft(){
  //x motor anitclockwise
    digitalWrite(XDIR_PIN, LOW);
  digitalWrite(YDIR_PIN, LOW);
   
  //x motor 
    for(int i = 0; i < 200; i++) {

    digitalWrite(XSTEP_PIN, HIGH);
    delayMicroseconds(800);

    digitalWrite(XSTEP_PIN, LOW);
    delayMicroseconds(800);

}
}

void stepperCenter(){
  //x motor anitclockwise
    digitalWrite(XDIR_PIN, LOW);
   
  //x motor 
    for(int i = 0; i < 225; i++) {

    digitalWrite(XSTEP_PIN, HIGH);
    delayMicroseconds(800);

    digitalWrite(XSTEP_PIN, LOW);
    delayMicroseconds(800);

}
}

void stepperUp(){
  //y motor 
    digitalWrite(XDIR_PIN, HIGH);
  digitalWrite(YDIR_PIN, HIGH);
   
  //x motor 
    for(int i = 0; i < 550; i++) {

    digitalWrite(YSTEP_PIN, HIGH);
    delayMicroseconds(800);

    digitalWrite(YSTEP_PIN, LOW);
    delayMicroseconds(800);

}
}

void stepperDown(){
  //y motor anticlockwise
    digitalWrite(XDIR_PIN, LOW);
  digitalWrite(YDIR_PIN, LOW);
   
  //x motor 
    for(int i = 0; i < 550; i++) {

    digitalWrite(YSTEP_PIN, HIGH);
    delayMicroseconds(800);

    digitalWrite(YSTEP_PIN, LOW);
    delayMicroseconds(800);

}
}
//stepper code
void stepperControl()
{
  digitalWrite(XDIR_PIN, HIGH);
  digitalWrite(YDIR_PIN, HIGH);
//x motor
  for(int i = 0; i < 550; i++) {

    digitalWrite(XSTEP_PIN, HIGH);
    delayMicroseconds(800);

    digitalWrite(XSTEP_PIN, LOW);
    delayMicroseconds(800);
  }

  //y motor
  for(int i=0 ; i<400 ; i++){
        digitalWrite(YSTEP_PIN, HIGH);
        delayMicroseconds(800);

         digitalWrite(YSTEP_PIN, LOW);
         delayMicroseconds(800);


  }

  delay(1000);

  // Anti-clockwise
  digitalWrite(XDIR_PIN, LOW);
  digitalWrite(YDIR_PIN, LOW);

//x motor
  for(int i = 0; i < 550; i++) {

    digitalWrite(XSTEP_PIN, HIGH);
    delayMicroseconds(800);

    digitalWrite(XSTEP_PIN, LOW);
    delayMicroseconds(800);
  }

  //y motor
  for(int i=0 ; i<400 ; i++){
        digitalWrite(YSTEP_PIN, HIGH);
        delayMicroseconds(800);

         digitalWrite(YSTEP_PIN, LOW);
         delayMicroseconds(800);


  }


  delay(1000);
}



struct ColorData {
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t c;
};

void setMuxChannel(uint8_t channel) {
  digitalWrite(SENSOR_LED_PIN, HIGH); 

  Wire.beginTransmission(PCA_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

ColorData readAveragedColor(uint8_t channel) {
  uint32_t rTotal = 0;
  uint32_t gTotal = 0;
  uint32_t bTotal = 0;
  uint32_t cTotal = 0;

  setMuxChannel(channel);
  delay(20);

  for (int i = 0; i < 5; i++) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);

    rTotal += r;
    gTotal += g;
    bTotal += b;
    cTotal += c;

    delay(5);
  }

  ColorData d;
  d.r = rTotal / 5;
  d.g = gTotal / 5;
  d.b = bTotal / 5;
  d.c = cTotal / 5;

  return d;
}

String detectColor(ColorData d) {
  if (d.c < 60) {
    return "NO_OBJECT";
  }

  float rf = (float)d.r / d.c;
  float gf = (float)d.g / d.c;
  float bf = (float)d.b / d.c;

  if (bf > 0.38 && bf > rf + 0.10 && bf > gf + 0.10) {
    return "BLUE";
  }

  if (rf > 0.32 && rf > bf + 0.12 && rf > gf + 0.12) {
    return "RED";
  }

  return "UNKNOWN";
}

void senseColor(){
  for (uint8_t ch = 0; ch < COLOR_SENSOR_COUNT; ch++) {
    ColorData d = readAveragedColor(ch);
    String color = detectColor(d);

    Serial.print("Sensor ");
    Serial.print(ch + 1);
    Serial.print(" -> R: ");
    Serial.print(d.r);
    Serial.print(" | G: ");
    Serial.print(d.g);
    Serial.print(" | B: ");
    Serial.print(d.b);
    Serial.print(" | C: ");
    Serial.print(d.c);
    Serial.print(" => ");
    Serial.println(color);
  }

  Serial.println("------------------------------------");
  delay(250); 
}

void servo1To0() {
  servo1.write(0);
}

void servo1To90() {
  servo1.write(90);
}

// -------- Servo 2 functions --------
void servo2To0() {
  servo2.write(0);
}

void servo2To30() {
  servo2.write(35);
}


void servoShoot(){

  servo1To90();
  delay(2000);

  servo2To30();
  delay(2000);

  servo1To0();
  delay(2000);

  servo2To0();
  delay(2000);
}

void checkColorSensor1Red() {


  uint16_t r, g, b, c;

  // Check Sensor 1
  setMuxChannel(0);
  tcs.getRawData(&r, &g, &b, &c);

  Serial.print("Sensor 1 Red Value: ");
  Serial.println(r);

  if (r > colorThreshold) {
    Serial.println("Sensor 1 red detected");
    stepperRight();
    servoShoot();
  }

  delay(100);

  // Check Sensor 2
  setMuxChannel(1);
  tcs.getRawData(&r, &g, &b, &c);

  Serial.print("Sensor 2 Red Value: ");
  Serial.println(r);

  if (r > colorThreshold) {
    Serial.println("Sensor 2 red detected");
    stepperLeft();
    servoShoot();
    
  }
}


void lineFollowForwardOnWhite() {
  int L2 = analogRead(FLMS);
  int L1 = analogRead(FLS);
  int C  = analogRead(FCS);
  int R1 = analogRead(FRS);
  int R2 = analogRead(FRMS);

  bool fL2 = (L2 < threshold);
  bool fL1 = (L1 < threshold);
  bool fC  = (C  < threshold);
  bool fR1 = (R1 < threshold);
  bool fR2 = (R2 < threshold);

  if (fC) setMotor(baseSpeed - 5, baseSpeed);
  else if (fR1) setMotor(turnSpeed, baseSpeed);
  else if (fR2) setMotor(0, baseSpeed);
  else if (fL1) setMotor(baseSpeed, turnSpeed);
  else if (fL2) setMotor(baseSpeed, 0);
  else setMotor(baseSpeed - 5, baseSpeed);  // all white: keep forward
}


void encoderTicckcount(int targetStrips) {
  if (targetStrips <= 0) {
    stopMotors();
    return;
  }

  int stripCount = 0;
  bool prevStripDetected = false;
  unsigned long lastStripTime = 0;

  while (true) {
    bool stripDetected = (digitalRead(leftCounter) == HIGH &&
                          digitalRead(rightCounter) == HIGH);

    if (stripDetected && !prevStripDetected &&
        (stripCount == 0 || millis() - lastStripTime > debounceTime)) {
      stripCount++;
      lastStripTime = millis();

      Serial.print(">>> Strip: ");
      Serial.println(stripCount);

      if (stripCount >= targetStrips) {
        stopMotors();
        Serial.println(">>> Target strips reached. Stopping.");
        return;
      }
    }

    prevStripDetected = stripDetected;

    lineFollowForwardOnWhite();
    delay(20);
  }
}


void encoderTickCount(long c) {
  encoderTicckcount(c);
}

// -------- Servo 1 functions --------



// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(9600);
  Wire.begin();
  pinMode(SENSOR_LED_PIN, OUTPUT);

  for (uint8_t ch = 0; ch < COLOR_SENSOR_COUNT; ch++) {
    setMuxChannel(ch);
    delay(50);

    Serial.print("Sensor ");
    Serial.print(ch + 1);
    if (tcs.begin()) {
      Serial.println(" ready!");
    } else {
      Serial.println(" fail!");
    }
  }


  pinMode(M1_RPWM, OUTPUT);
  pinMode(M1_LPWM, OUTPUT);
  pinMode(M2_RPWM, OUTPUT);
  pinMode(M2_LPWM, OUTPUT);

  pinMode(LEFT_ENCODER, INPUT_PULLUP);
  pinMode(RIGHT_ENCODER, INPUT_PULLUP);

  pinMode(leftCounter, INPUT);
  pinMode(rightCounter, INPUT);

  pinMode(XSTEP_PIN, OUTPUT);
  pinMode(XDIR_PIN, OUTPUT);
  pinMode(YSTEP_PIN, OUTPUT);
  pinMode(YDIR_PIN, OUTPUT);


  attachInterrupt(digitalPinToInterrupt(LEFT_ENCODER), leftEncoderISR, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_ENCODER), rightEncoderISR, RISING);

  // servo1.attach(SERVO_PIN1);
  // servo2.attach(SERVO_PIN2);

  // // FORCE START POSITION
  // servo1.write(0);
  // servo2.write(0);
  // delay(1000);   

  stopMotors();

  Serial.println("Robot Ready");

//    lineFollowUntil(3);
//  delay(1000);

// stepperLeft();
// stepperRight();
// stepperUp();
// stepperDown();

stepperCenter();
  
//     digitalWrite(XDIR_PIN, HIGH);
//   digitalWrite(YDIR_PIN, HIGH);
// //x motor
//   for(int i = 0; i < 550; i++) {

//     digitalWrite(XSTEP_PIN, HIGH);
//     delayMicroseconds(800);

//     digitalWrite(XSTEP_PIN, LOW);
//     delayMicroseconds(800);
//   }


//    servo1To0();
//   delay(2000);


//   servo1To90();
//   delay(2000);

//   servo2To30();
//   delay(2000);

//   servo1To0();
//   delay(2000);

//   servo2To0();
//   delay(2000);


  
//         // Anti-clockwise
//   digitalWrite(XDIR_PIN, LOW);
//   digitalWrite(YDIR_PIN, LOW);

// //x motor
//   for(int i = 0; i < 450; i++) {

//     digitalWrite(XSTEP_PIN, HIGH);
//     delayMicroseconds(800);

//     digitalWrite(XSTEP_PIN, LOW);
//     delayMicroseconds(800);
//   }

//      servo1To0();
//   delay(2000);


//   servo1To90();
//   delay(2000);

//   servo2To30();
//   delay(2000);

//   servo1To0();
//   delay(2000);

//   servo2To0();
//   delay(2000);

//   lineFollowUntil(3);
//  delay(1000);
//  B_lineFollowUntil(2);
//  delay(1000);
//  encoderLeft(200);
// encoderRight(200);
//  delay(1000);
//    lineFollowUntil(3);
//  delay(1000);
//   lineFollowUntil(1);
//  delay(1000);
//   encoderLeft(205);
//  delay(1000);
//  encoderForward(1000);
//  delay(1000);

// encoderTicks(500);
// encoderTickCount(100);

//  forward();

// encoderTicckcount(2);



}

// =====================================
// LOOP TEST
// =====================================
void loop() {

senseColor();
  //  printForwardSensors();
  //   Serial.println("");
   
  // Move forward until 3 junctions
  // lineFollowUntil(2);
  // delay(500);

//  encoderLeft(200);
//  delay(600);

  // delay(2000);

  // You can later add backward version similarly


//     digitalWrite(XDIR_PIN, HIGH);
//   digitalWrite(YDIR_PIN, HIGH);
// // x motor
//   for(int i = 0; i < 550; i++) {

//     digitalWrite(XSTEP_PIN, HIGH);
//     delayMicroseconds(800);

//     digitalWrite(XSTEP_PIN, LOW);
//     delayMicroseconds(800);
//   }

//         // Anti-clockwise
//   digitalWrite(XDIR_PIN, LOW);
//   digitalWrite(YDIR_PIN, LOW);

// //x motor
//   for(int i = 0; i < 550; i++) {

//     digitalWrite(XSTEP_PIN, HIGH);
//     delayMicroseconds(800);

//     digitalWrite(XSTEP_PIN, LOW);
//     delayMicroseconds(800);
//   }

}
