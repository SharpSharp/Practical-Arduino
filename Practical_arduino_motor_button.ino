/*  Arduino Test Track
 *  
 *  A simple turnout with sensors servos and signals
 *   
 */

/* definging readable names for the I/O pins */

#define frontSensorPin  2
#define turnoutServoPin  6                //PWM pin
#define turnoutThrowPin 7
#define directionPin 8
#define frogRelayPin 13
#define signalServoPin 5           //PWM pin
#define divergeSensorPin 4
#define throughSensorPin 3
#define signalGreenPin 10          //PWM pin
#define signalRedPin   9           //PWM pin

/* Pins used to drive the motor */
#define motorPinA 11                      //connect to A-IA
#define motorPinB 12                      //connect to A-IB
#define throttlePin A5                    // Analogue input from Pot

/* Setup Pot to work as throttle */
#define throttleMin 0
#define throttleMax 1024

/* Setup servo limits */
#define closedPosition 50      // angle
#define thrownPosition 90      // angle
#define dangerPosition  80      // angle
#define proceedPosition 150     // angle
#define turnoutThrowSpeed 30  // [ms] lower number is faster

#include<Servo.h>
/* assign servos */
Servo turnoutServo;
Servo signalServo;

#include "HCMotor.h"
/* Create an instance of the library */
HCMotor HCMotor;

byte turnoutAngle   = closedPosition;
byte turnoutState   = closedPosition;
unsigned long turnoutThrowTime;
byte signalPosition;
bool turnoutClosed = true;
bool greenAspect = false;
bool redAspect = true;
bool directionForward = true;


// two fuctions to throw and close the turnout
void throwTurnout(){
  for (turnoutAngle = closedPosition; turnoutAngle <= thrownPosition; turnoutAngle++) {
    turnoutServo.write(turnoutAngle);
    delay(turnoutThrowSpeed);
  }
  turnoutClosed = false;
  digitalWrite(frogRelayPin, turnoutClosed);
}

void closeTurnout(){
  for (turnoutAngle = thrownPosition; turnoutAngle >= closedPosition; turnoutAngle--) {
    turnoutServo.write(turnoutAngle);
    delay(turnoutThrowSpeed);
  }
  turnoutClosed = true;
  digitalWrite(frogRelayPin, turnoutClosed);
}

void signalToDanger(){
  if (signalPosition != dangerPosition){
    for (signalPosition = proceedPosition; signalPosition > dangerPosition; signalPosition--) { 
      signalServo.write(signalPosition);
      delay(6);
    }
    for (signalPosition = dangerPosition; signalPosition < dangerPosition+9; signalPosition++) { 
      signalServo.write(signalPosition); 
      delay(7);
    }
    for (signalPosition = dangerPosition+10; signalPosition > dangerPosition; signalPosition--) { 
      signalServo.write(signalPosition);
      delay(15);
    }
  }
}

void signalToProceed(){
  if (signalPosition != proceedPosition){  
    for (signalPosition = dangerPosition; signalPosition < proceedPosition; signalPosition++) { 
      signalServo.write(signalPosition); 
      delay(7);
    }
  }
}

// Function to control the two aspect colour signal
void colourSignalDanger(bool danger){
  if (danger){
    greenAspect = false;
    redAspect   = true;
  }
  else {
    greenAspect = true;
    redAspect   = false;    
  }
  digitalWrite(signalGreenPin, greenAspect);
  digitalWrite(signalRedPin, redAspect);
}


void setup() {
  // put your setup code here, to run once:
  pinMode(divergeSensorPin, INPUT);
  pinMode(throughSensorPin, INPUT);
  pinMode(frontSensorPin, INPUT);
  
  pinMode(turnoutThrowPin, INPUT_PULLUP);
  pinMode(directionPin, INPUT_PULLUP);
  
  pinMode(frogRelayPin, OUTPUT);
  
  pinMode(signalGreenPin, OUTPUT);
  pinMode(signalRedPin, OUTPUT);

  /* initailise Signal and its position */
  signalServo.attach(signalServoPin);
  signalServo.write(proceedPosition);
  
  /* initailise turnout and its position */
  turnoutServo.attach(turnoutServoPin);
  turnoutServo.write(closedPosition);
  
  /* initailise colour signal and frog positions */
  digitalWrite(signalGreenPin, greenAspect);
  digitalWrite(signalRedPin, redAspect);
  digitalWrite(frogRelayPin, turnoutClosed);

  /* Initialise the library */
  HCMotor.Init();

  /* Attach motor 0 to digital pins. The first parameter specifies the 
     motor number, the second is the motor type, and the third and forth are the 
     digital pins that will control the motor */
  HCMotor.attach(0, DCMOTOR_H_BRIDGE, motorPinA, motorPinB);

  /* Set the duty cycle of the PWM signal in 100uS increments. 
     Here 100 x 100uS = 1mS duty cycle. */
  HCMotor.DutyCycle(0, 100);

  Serial.begin(9600);
}

void loop() {
  if (digitalRead(turnoutThrowPin) == LOW){
    if (turnoutClosed){
      signalToDanger();
      delay(1000);
      throwTurnout();
      delay(500);
      colourSignalDanger(false);
    }
    else {
      colourSignalDanger(true);
      delay(100);
      closeTurnout();
      delay(1000);
      signalToProceed();
    }
  }
  
  if(digitalRead(frontSensorPin) == LOW){
    if (turnoutClosed){
      signalToDanger();
    } else {
      colourSignalDanger(true);
    }
  }
  
  if(digitalRead(throughSensorPin) == LOW && turnoutClosed) signalToProceed();
  if(digitalRead(divergeSensorPin) == LOW && !turnoutClosed) colourSignalDanger(false);
  
  int throttleSpeed, Pot;


  Pot = analogRead(throttlePin); // read throttle pin
  
  throttleSpeed = map(Pot, throttleMin, throttleMax, 0, 100);
  
  Serial.println(throttleSpeed);
  /* Set the on time of the duty cycle to match the position of the throttle. */
  HCMotor.OnTime(0, throttleSpeed);

  /* read directionPin and change direction when pressed */
  if(digitalRead(directionPin) == LOW){
    if (directionForward == false){
      directionForward = true;
      HCMotor.Direction(0, FORWARD);
    } 
    else {
      directionForward = false;
      HCMotor.Direction(0, REVERSE);
    }
    delay(200); 
  }
}
