#include "IR_remote.h"
#include "keymap.h"
#include <Servo.h>

// Pin Definitions
#define MOTOR_DIR_1    2
#define MOTOR_DIR_2    4
#define MOTOR_PWM_1    5
#define MOTOR_PWM_2    6
#define LINE_SENSOR_1  7
#define LINE_SENSOR_2  8
#define LINE_SENSOR_3  9
#define SERVO_PIN      10
#define ULTRASONIC_TRIG 12
#define ULTRASONIC_ECHO 13
#define IR_PIN         3

// Constants
#define DEFAULT_SPEED  110
#define MAX_SPEED      255
#define MIN_SPEED      0
#define SERVO_MIN      0
#define SERVO_MAX      180
#define SERVO_CENTER   90
#define SERVO_STEP     3
#define BLE_BUFFER_SIZE 10

// Sensor Constants
#define BLACK 1
#define WHITE 0

// Objects
IRremote ir(IR_PIN);
Servo cameraServo;

// Global Variables
String bleValue = "";
String bleValueTemp = "";
int servoAngle = SERVO_CENTER;
char irCarMode = ' ';
bool irModeFlag = false;
int leftTraValue = 1;
int centerTraValue = 1;
int rightTraValue = 1;
float frontDistance = 0;

// Motion control variables
unsigned long motionStartTime = 0;
unsigned long motionDuration = 0;
bool isInMotion = false;
char currentMotionCommand = ' ';
char activeMode = ' '; // T=Tracing, A=Avoidance, Z=Follow, ' '=Manual

//--------------------------------
// Motor Control Functions
//--------------------------------
void moveForward(int speed) {
  digitalWrite(MOTOR_DIR_1, HIGH);
  analogWrite(MOTOR_PWM_1, speed);
  digitalWrite(MOTOR_DIR_2, LOW);
  analogWrite(MOTOR_PWM_2, speed);
}

void moveBackward(int speed) {
  digitalWrite(MOTOR_DIR_1, LOW);
  analogWrite(MOTOR_PWM_1, speed);
  digitalWrite(MOTOR_DIR_2, HIGH);
  analogWrite(MOTOR_PWM_2, speed);
}

void rotateLeft(int speed) {
  digitalWrite(MOTOR_DIR_1, LOW);
  analogWrite(MOTOR_PWM_1, speed);
  digitalWrite(MOTOR_DIR_2, LOW);
  analogWrite(MOTOR_PWM_2, speed);
}

void rotateRight(int speed) {
  digitalWrite(MOTOR_DIR_1, HIGH);
  analogWrite(MOTOR_PWM_1, speed);
  digitalWrite(MOTOR_DIR_2, HIGH);
  analogWrite(MOTOR_PWM_2, speed);
}

void stopMotors() {
  digitalWrite(MOTOR_DIR_1, LOW);
  analogWrite(MOTOR_PWM_1, 0);
  digitalWrite(MOTOR_DIR_2, HIGH);
  analogWrite(MOTOR_PWM_2, 0);
}

//--------------------------------
// Ultrasonic Sensor
//--------------------------------
float checkDistance() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);
  
  float distance = pulseIn(ULTRASONIC_ECHO, HIGH) / 58.00;
  delay(10);
  return distance;
}

//--------------------------------
// Servo Control
//--------------------------------
void setServoAngle(int angle) {
  servoAngle = constrain(angle, SERVO_MIN, SERVO_MAX);
  cameraServo.write(servoAngle);
}

void moveServoUp(int step = 4) {
  setServoAngle(servoAngle + step);
}

void moveServoDown(int step = 4) {
  setServoAngle(servoAngle - step);
}

//--------------------------------
// Line Following (Infrared Tracing)
//--------------------------------
void infraredTracing() {
  leftTraValue = digitalRead(LINE_SENSOR_1);
  centerTraValue = digitalRead(LINE_SENSOR_2);
  rightTraValue = digitalRead(LINE_SENSOR_3);
  
  if (leftTraValue != BLACK && centerTraValue == BLACK && rightTraValue != BLACK) {
    // On line - go forward
    moveForward(120);
  } 
  else if (leftTraValue == BLACK && centerTraValue == BLACK && rightTraValue != BLACK) {
    // Slight left
    rotateLeft(80);
  } 
  else if (leftTraValue == BLACK && centerTraValue != BLACK && rightTraValue != BLACK) {
    // Sharp left
    rotateLeft(120);
  } 
  else if (leftTraValue != BLACK && centerTraValue != BLACK && rightTraValue == BLACK) {
    // Sharp right
    rotateRight(120);
  } 
  else if (leftTraValue != BLACK && centerTraValue == BLACK && rightTraValue == BLACK) {
    // Slight right
    rotateRight(80);
  } 
  else if (leftTraValue == BLACK && centerTraValue == BLACK && rightTraValue == BLACK) {
    // All sensors on black - stop
    stopMotors();
  }
}

//--------------------------------
// Ultrasonic Obstacle Avoidance (Improved)
//--------------------------------
unsigned long avoidanceTimer = 0;
int avoidanceState = 0; // 0=forward, 1=backward, 2=turning

void ultrasonicAvoidance() {
  frontDistance = checkDistance();
  
  if (avoidanceState == 0) {
    // Normal forward state
    if (frontDistance <= 10) {
      moveBackward(100);
      avoidanceTimer = millis();
      avoidanceState = 1;  
    }  
    else if (frontDistance > 10 && frontDistance <= 20) {
      // Random turn
      if (random(1, 100) >= 50) {
        rotateLeft(100);
      } else {
        rotateRight(100);
      }
      avoidanceTimer = millis();
      avoidanceState = 2;
    } 
    else if (frontDistance > 20) {
      moveForward(70);
    }
  }
  else if (avoidanceState == 1) {
    // Backing up
    if (millis() - avoidanceTimer >= 200) {
      // Random turn
      if (random(1, 100) >= 50) {
        rotateLeft(100);
      } else {
        rotateRight(100);
      }
      avoidanceTimer = millis();
      avoidanceState = 2;
    }
  }
  else if (avoidanceState == 2) {
    // Turning
    if (millis() - avoidanceTimer >= 500) {
      avoidanceState = 0;
    }
  }
}

//--------------------------------
// Ultrasonic Follow Mode
//--------------------------------
void ultrasonicFollow() {
  frontDistance = checkDistance();
  
  if (frontDistance <= 5) {
    moveBackward(80);
  } 
  else if (frontDistance > 5 && frontDistance <= 10) {
    stopMotors();
  } 
  else if (frontDistance > 20) {
    moveForward(100);
  }
}

//--------------------------------
// Motion Control with Timing
//--------------------------------
void startMotion(char command, unsigned long duration) {
  currentMotionCommand = command;
  motionStartTime = millis();
  motionDuration = duration;
  isInMotion = true;
  
  switch (command) {
    case 'f':
      moveForward(110);
      break;
    case 'b':
      moveBackward(110);
      break;
    case 'l':
      rotateLeft(110);
      break;
    case 'r':
      rotateRight(110);
      break;
    case 's':
      stopMotors();
      isInMotion = false;
      break;
  }
}

void updateMotion() {
  if (isInMotion && (millis() - motionStartTime >= motionDuration)) {
    stopMotors();
    isInMotion = false;
    currentMotionCommand = ' ';
  }
}

//--------------------------------
// IR Remote Control
//--------------------------------
void irRemoteControl() {
  // Execute pending IR command
  if (irCarMode != ' ' && !isInMotion) {
    switch (irCarMode) {
      case 'f':
        startMotion('f', 300);
        irCarMode = ' ';
        break;
      case 'b':
        startMotion('b', 300);
        irCarMode = ' ';
        break;
      case 'l':
        startMotion('l', 300);
        irCarMode = ' ';
        break;
      case 'r':
        startMotion('r', 300);
        irCarMode = ' ';
        break;
      case 's':
        stopMotors();
        irCarMode = ' ';
        break;
      case '+':
        moveServoUp(3);
        irCarMode = ' ';
        break;
      case '-':
        moveServoDown(3);
        irCarMode = ' ';
        break;
    }
  }
  
  // Check for new IR input
  int key = ir.getIrKey(ir.getCode(), 1);

  if (key == IR_KEYCODE_UP) {
    irCarMode = 'f';
    irModeFlag = true;
  } 
  else if (key == IR_KEYCODE_DOWN) {
    irCarMode = 'b';
    irModeFlag = true;
  } 
  else if (key == IR_KEYCODE_LEFT) {
    irCarMode = 'l';
    irModeFlag = true;
  } 
  else if (key == IR_KEYCODE_RIGHT) {
    irCarMode = 'r';
    irModeFlag = true;
  } 
  else if (key == IR_KEYCODE_OK) {
    irCarMode = 's';
    irModeFlag = true;
  } 
  else if (key == IR_KEYCODE_2) {
    irCarMode = '+';
    irModeFlag = true;
  } 
  else if (key == IR_KEYCODE_8) {
    irCarMode = '-';
    irModeFlag = true;
  }
}

//--------------------------------
// Bluetooth Command Processing
//--------------------------------
void processBLECommand() {
  if (bleValue.length() == 0) return;
  
  // Check valid command format: %X#
  if (bleValue.length() <= 4) {

    if (bleValue.charAt(0) == '%') {
      
      // Stop IR mode if BLE command received
      if (irModeFlag == true) {
        stopMotors();
        irCarMode = ' ';
        irModeFlag = false;
        isInMotion = false;
      }
      
      char command = bleValue.charAt(1);

      Serial.println(command);
      
      switch(command) {
        case 'H':
          moveServoUp(4);
          bleValue = "";
          break;
        case 'G':
          moveServoDown(4);
          bleValue = "";
          break;
        case 'F':
          if (!isInMotion) {
            startMotion('f', 400);
            bleValue = "";
          }
          break;
        case 'B':
          if (!isInMotion) {
            startMotion('b', 400);
            bleValue = "";
          }
          break;
        case 'L':
          if (!isInMotion) {
            startMotion('l', 250);
            bleValue = "";
          }
          break;
        case 'R':
          if (!isInMotion) {
            startMotion('r', 250);
            bleValue = "";
          }
          break;
        case 'T':
          activeMode = 'T';
          bleValue = "";
          break;
        case 'A':
          activeMode = 'A';
          bleValue = "";
          break;
        case 'Z':
          activeMode = 'Z';
          bleValue = "";
          break;
        case 'S':
          stopMotors();
          activeMode = ' ';
          isInMotion = false;
          bleValue = "";
          break;
        default:
          bleValue = "";
          break;
      }
    }
  } else {
    bleValue = "";
  }
}
//--------------------------------
// Setup
//--------------------------------
void setup() {
  // Initialize Serial Communication
  Serial.begin(115200);
  
  // Initialize Servo
  cameraServo.attach(SERVO_PIN);
  cameraServo.write(servoAngle);
  
  // Motor Pins
  pinMode(MOTOR_DIR_1, OUTPUT);
  pinMode(MOTOR_DIR_2, OUTPUT);
  pinMode(MOTOR_PWM_1, OUTPUT);
  pinMode(MOTOR_PWM_2, OUTPUT);
  
  // Line Sensor Pins
  pinMode(LINE_SENSOR_1, INPUT);
  pinMode(LINE_SENSOR_2, INPUT);
  pinMode(LINE_SENSOR_3, INPUT);
  
  // Ultrasonic Pins
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  
  // Initialize motors to stop
  stopMotors();
  
  // Startup delay
  delay(100);
}

//--------------------------------
// Main Loop
//--------------------------------
void loop() {
  // Read Bluetooth data (non-blocking)
  while (Serial.available() > 0) {
    bleValueTemp += (char)Serial.read();

    delay(2);
  
    if (!Serial.available()) {
      bleValue = bleValueTemp;
      bleValueTemp = "";
    }
  }
  
  // Update motion timer
  updateMotion();
  
  // Process Bluetooth command
  if (bleValue.length() > 0) {
    processBLECommand();
  }
  
  // Handle active modes (only when not in timed motion)
  if (!isInMotion) {
    switch (activeMode) {
      case 'T':
        infraredTracing();
        break;
      case 'A':
        ultrasonicAvoidance();
        break;
      case 'Z':
        ultrasonicFollow();
        break;
      default:
        // Manual mode - only stop if no command pending
        if (bleValue.length() == 0 && irCarMode == ' ' && !irModeFlag) {
          // Don't call stopMotors() repeatedly - only when needed
        }
        break;
    }
  }
  
  // Handle IR remote
  irRemoteControl();
  
  // Small delay for stability
  delay(5);
}
