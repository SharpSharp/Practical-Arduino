/*  Arduino Test Track
 *  
 *  A simple turnout with sensors servos and signals
 *  driving an auto-shuttle.
 *   
 */

/* definging readable names for the I/O pins */

#define frontSensorPin    2
#define throughSensorPin  3           //PWM pin
#define divergeSensorPin  4
#define signalServoPin    5           //PWM pin
#define turnoutServoPin   6           //PWM pin
#define turnoutThrowPin   7
#define turnoutClosePin   8
#define signalRedPin      9           //PWM pin
#define signalGreenPin    10          //PWM pin
#define frogRelayPin      18

/* Pins used to drive the motor */
#define motorPinA 11                  //connect to A-IA
#define motorPinB 12                  //connect to A-IB
#define throttlePin A5                // Analogue input from Pot

/* Setup Pot to work as throttle */
#define throttleMin 0
#define throttleMax 1023

/* Setup servo limits */
#define turnoutClosedPosition 50      // angle
#define turnoutThrownPosition 90      // angle
#define turnoutMoveSpeed 30           // [ms] lower number is faster


#define signalDanger  80              // angle
#define signalProceed 150             // angle
#define signalThrowSpeed 5            // [ms] lower number is faster

#define decelerationRate 20           // ms
#define accelerationRate 30           // ms

#include<Servo.h>
/* assign servos */
Servo turnoutServo;
Servo throughSignalServo;

#include "HCMotor.h"
/* Create an instance of the library */
HCMotor HCMotor;


byte turnoutPosition   = turnoutClosedPosition;
byte turnoutTarget   = turnoutClosedPosition;
byte turnoutTransition = 0;
bool turnoutIsClosed = true;
unsigned long turnoutMoveDelay;
//unsigned long turnoutThrowTime = turnoutMoveSpeed * (turnoutThrownPosition-turnoutClosedPosition);

/* semaphore stuff */
byte signalPosition   = signalDanger;
byte signalTarget   = signalDanger;
unsigned long signalThrowDelay;
//unsigned long signalThrowTime = signalThrowSpeed * (signalProceed-signalDanger);


/* throttle related stuff */
int tramTransition = 0;
byte tramState = 1;
int throttleSpeed = 50;
int minSpeed = 0;
int currentSpeed, Pot, targetSpeed;
unsigned long changeSpeedDelay = 20;
unsigned long timeToReStart;
unsigned long startDelay = 4000; // Note: this could be calculated if desired


/* call functions */

// Function to control the two aspect colour signal
void colourSignalDanger(bool danger){
  digitalWrite(signalRedPin, danger);
  digitalWrite(signalGreenPin, !danger);
}

// function to throw turnout
void throwTurnout(){
  turnoutMoveDelay = millis() + turnoutMoveSpeed;
  if (turnoutPosition < turnoutTarget) turnoutPosition++;
  if (turnoutPosition > turnoutTarget) turnoutPosition--;
  turnoutServo.write(turnoutPosition);
  if (turnoutPosition == turnoutThrownPosition) {
    turnoutIsClosed = false;
    digitalWrite(frogRelayPin, turnoutIsClosed);
  }
  if (turnoutPosition == turnoutClosedPosition) {
    turnoutIsClosed = true;
    digitalWrite(frogRelayPin, turnoutIsClosed);
  }

}

// function to move signal
void moveSignal(){
  signalThrowDelay = millis() + signalThrowSpeed;
  if (signalPosition < signalTarget) signalPosition++;
  if (signalPosition > signalTarget) signalPosition--;
  throughSignalServo.write(signalPosition);
}


// function for acceleration and deceleration
void changeSpeed(){
  if (currentSpeed > targetSpeed){
    currentSpeed--;
    changeSpeedDelay = millis() + decelerationRate;  
  }
  if (currentSpeed < targetSpeed) {
    currentSpeed++;
    changeSpeedDelay = millis() + accelerationRate;
  }
  HCMotor.OnTime(0, currentSpeed);
}


void setup() {
  // put your setup code here, to run once:
  pinMode(frontSensorPin, INPUT);
     
  pinMode(turnoutThrowPin, INPUT_PULLUP);
  pinMode(turnoutClosePin, INPUT_PULLUP);
  
  pinMode(frogRelayPin, OUTPUT);

  pinMode(throughSensorPin, INPUT);
  
  pinMode(divergeSensorPin, INPUT);
  pinMode(signalGreenPin, OUTPUT);
  pinMode(signalRedPin, OUTPUT);
  
  /* initailise Signal and its position */
  throughSignalServo.attach(signalServoPin);
  throughSignalServo.write(signalDanger);
  

  /* initailise turnout and its position */
  turnoutServo.attach(turnoutServoPin);
  turnoutServo.write(turnoutClosedPosition);
  
  /* initailise colour signal and frog positions */
  colourSignalDanger(true);
  digitalWrite(frogRelayPin, turnoutIsClosed);

  /* Initialise the library */
  HCMotor.Init();

  /* Attach motor 0 to digital pins. The first parameter specifies the 
     motor number, the second is the motor type, and the third and forth are the 
     digital pins that will control the motor */
  HCMotor.attach(0, DCMOTOR_H_BRIDGE, motorPinA, motorPinB);

  /* Set the duty cycle of the PWM signal in 100uS increments. 
     Here 100 x 100uS = 1mS duty cycle. */
  HCMotor.DutyCycle(0, 100);
  
   
  Pot = analogRead(throttlePin);                                  // read throttle pin
  throttleSpeed = map(Pot, throttleMin, throttleMax, 0, 100);     // map the pot value 

  currentSpeed = throttleSpeed;
  targetSpeed = throttleSpeed;
  
  /* Set the on time of the duty cycle to match the position of the throttle. */
  HCMotor.Direction(0, FORWARD);
  HCMotor.OnTime(0, throttleSpeed);
  
}

void loop() {
  Pot = analogRead(throttlePin);								                // read throttle pin
  throttleSpeed = map(Pot, throttleMin, throttleMax, 0, 100);   // map the pot value 
  
  switch (tramState){
    case 1: //  moving forward waiting for a through or diverge sensor
      if(digitalRead(divergeSensorPin) == LOW) tramTransition = 12;
      if(digitalRead(throughSensorPin) == LOW) tramTransition = 12;
    break;

    case 2: // slowing to a halt at twin end
      if (currentSpeed == 0) tramTransition = 23;
    break;
    
    case 3: // stopped at twin end, waiting for timer
      if (millis() > timeToReStart) tramTransition = 34;
    break;
    
    case 4: // moving backwards waiting for a sensor at the front
      if(digitalRead(frontSensorPin) == LOW)tramTransition = 45;
    break;

    case 5: // slowing to a halt at front
      if (currentSpeed == 0) tramTransition = 56;
    break;
    
    case 6: //  stopped at front, waiting for timer
      if (millis() > timeToReStart) tramTransition = 61;
    break;
  }
 
  switch (tramTransition){
    case 12:                              // tram reached twin end sensor. stop it here 
      targetSpeed = minSpeed;             // slow to stop
//      slowingDelay = millis() + 200;			// allow to coast after sensor
      tramState = 2;
      tramTransition = 0;
    break;

    case 23:                                 // stopped at twin end, wait here.
      timeToReStart = millis() + startDelay;  // set up station dwell time
      tramState = 3;
      tramTransition = 0;
      if (turnoutIsClosed){
        signalTarget = signalProceed;          //
      } else {
        colourSignalDanger(false);            // change singal to green
      }
    break;
    
    case 34:								          // twin end timer reached. start tram
      HCMotor.Direction(0, REVERSE);	// set diretion to forawrd
      targetSpeed = throttleSpeed;			    // set taget speed
      tramState = 4;
      tramTransition = 0;
    break;
    
    case 45:                            // tram reached single end. stop it here
      targetSpeed = minSpeed;           // slow to stop
//      slowingDelay = millis() + 200;    // allow to coast after sensor
      tramState = 5;
      tramTransition = 0;
    break;
    
    case 56:                                  // stopped at single end. wait here
      timeToReStart = millis() + startDelay;  // set up station dwell time
      tramState = 6;
      tramTransition = 0;
      if (turnoutIsClosed){             // throw turnout
        turnoutTarget = turnoutThrownPosition;
        signalTarget = signalDanger;
      } else {
        turnoutTarget = turnoutClosedPosition;
        colourSignalDanger(true);     // change singal to red
      }
    break;

    case 61:								            // single end timer reached. start tram
      HCMotor.Direction(0, FORWARD);		// set diretion to reverse
      targetSpeed = throttleSpeed;				    // set taget speed
      tramState = 1;
      tramTransition = 0;
    break;
  }

/* check if obejctes are at their targets and call functions if they are not */

  if (turnoutPosition != turnoutTarget && millis() > turnoutMoveDelay) throwTurnout();
  if (signalPosition  != signalTarget  && millis() > signalThrowDelay) moveSignal();
  if (currentSpeed    != targetSpeed   && millis() > changeSpeedDelay) changeSpeed();
  
}
