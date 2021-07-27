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
#define rearSensorPin     14
#define frogRelayPin      18
#define rearRelayPin      15
#define throughRelayPin   16
#define divergeRelayPin   17

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
bool turnoutIsClosed = true;
unsigned long turnoutMoveDelay;
//unsigned long turnoutThrowTime = turnoutMoveSpeed * (turnoutThrownPosition-turnoutClosedPosition);

/* semaphore stuff */
byte signalPosition   = signalDanger;
byte signalTarget   = signalDanger;
unsigned long signalThrowDelay;
//unsigned long signalThrowTime = signalThrowSpeed * (signalProceed-signalDanger);


/* throttle related stuff */
int tramTransition = 1011;    // start towards end of tramTransition switch
byte tramState = 11;
int throttleSpeed = 50;
int minSpeed = 0;
int currentSpeed, Pot, targetSpeed;
unsigned long changeSpeedDelay = 20;
unsigned long timeToReStart;
unsigned long startDelay = 4000;   // 4 second station dwell time


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
  /* set up pin modes for senors, pushbuttons, relays and LEDs */
  pinMode(frontSensorPin,   INPUT);
  pinMode(rearSensorPin,    INPUT);
  pinMode(throughSensorPin, INPUT);
  pinMode(divergeSensorPin, INPUT);
       
  pinMode(turnoutThrowPin,  INPUT_PULLUP);
  pinMode(turnoutClosePin,  INPUT_PULLUP);
  
  pinMode(frogRelayPin,     OUTPUT);
  pinMode(rearRelayPin,     OUTPUT);
  pinMode(throughRelayPin,  OUTPUT);
  pinMode(divergeRelayPin,  OUTPUT);

  pinMode(signalGreenPin,   OUTPUT);
  pinMode(signalRedPin,     OUTPUT);
  
  /* initailise Signal and its position */
  throughSignalServo.attach(signalServoPin);
  throughSignalServo.write(signalDanger);
  

  /* initailise turnout and its position */
  turnoutServo.attach(turnoutServoPin);
  turnoutServo.write(turnoutClosedPosition);
  
  /* initailise colour signal and frog positions */
  colourSignalDanger(true);
  digitalWrite(frogRelayPin, turnoutIsClosed);

  /* initailise relays for isolating sections
     starting with only the through track powered */
  digitalWrite(rearRelayPin,    false );
  digitalWrite(throughRelayPin, true);
  digitalWrite(divergeRelayPin, true );


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
    case 1: //  moving forward waiting for through sensor
      if(digitalRead(throughSensorPin) == LOW) tramTransition = 12;
    break;

    case 2: // slowing to a halt at through end
      if (currentSpeed == 0) tramTransition = 23;
    break;
    
    case 3: // stopped at through end, waiting for timer
      if (millis() > timeToReStart) tramTransition = 34;
    break;
    
    case 4: //  moving forward waiting for diverge sensor
      if(digitalRead(divergeSensorPin) == LOW) tramTransition = 45;
    break;

    case 5: // slowing to a halt at diverge end
      if (currentSpeed == 0) tramTransition = 56;
    break;
    
    case 6: // stopped at diverge end, waiting for timer
      if (millis() > timeToReStart) tramTransition = 67;
    break;
     
    case 7: // moving backwards from through waiting for a sensor at the rear
      if(digitalRead(rearSensorPin) == LOW) tramTransition = 78;
    break;

    case 8: // slowing to a halt at rear end
      if (currentSpeed == 0) tramTransition = 89;
    break;
    
    case 9: //  stopped at rear, waiting for timer
      if (millis() > timeToReStart) tramTransition = 910;
    break;

    case 10: // moving backwards from diverge waiting for a sensor at the front
      if(digitalRead(frontSensorPin) == LOW) tramTransition = 1011;
    break;

    case 11: // slowing to a halt at front end
      if (currentSpeed == 0) tramTransition = 1112;
    break;
    
    case 12: //  stopped at front, waiting for timer
      if (millis() > timeToReStart) tramTransition = 121;
    break;
  }
  
  switch (tramTransition){
    case 12:                              // tram reached through end sensor. stop it here 
      targetSpeed = minSpeed;             // slow to stop
      tramState = 2;
      tramTransition = 0;
    break;

    case 23:                                  // stopped at through end, wait here.
      timeToReStart = millis() + startDelay;  // set up station dwell time
      turnoutTarget = turnoutThrownPosition;  // throw turnout ready for other tram
      digitalWrite(throughRelayPin, false);   // switch off power for this tram
      digitalWrite(rearRelayPin,    true);    // power track for other tram
      
      tramState = 3;
      tramTransition = 0;
    break;
    
    case 34:                                 // through end timer reached. start tram
      targetSpeed = throttleSpeed;          // set taget speed
      tramState = 4;
      tramTransition = 0;
    break;

    case 45:                              // tram reached diverge end sensor. stop it here 
      targetSpeed = minSpeed;             // slow to stop
      tramState = 5;
      tramTransition = 0;
    break;

    case 56:                                  // stopped at diverge end, wait here.
      timeToReStart = millis() + startDelay;  // set up station dwell time
      turnoutTarget = turnoutClosedPosition;  // close turnout ready for other tram
      digitalWrite(divergeRelayPin, false);   // switch off power for this tram
      digitalWrite(throughRelayPin, true);    // power track for other tram
      signalTarget = signalProceed;
      tramState = 6;
      tramTransition = 0;
    break;
    
    case 67:                          // diverge end timer reached. start tram
      HCMotor.Direction(0, REVERSE);  // set diretion to reverse
      targetSpeed = throttleSpeed;    // set taget speed
      tramState = 7;
      tramTransition = 0;
    break;
    
    case 78:                            // tram reached rear end. stop it here
      targetSpeed = minSpeed;           // slow to stop
      tramState = 8;
      tramTransition = 0;
    break;
    
    case 89:                                  // stopped at rear end. wait here
      timeToReStart = millis() + startDelay;  // set up station dwell time
      turnoutTarget = turnoutThrownPosition;  // throw turnout ready for other tram
      digitalWrite(rearRelayPin,    false);   // switch off power for this tram
      digitalWrite(divergeRelayPin, true);    // power track for other tram
      signalTarget = signalDanger;
      colourSignalDanger(false);              // change singal to green
      tramState = 9;
      tramTransition = 0;
    break;

    case 910:								            // rear end timer reached. start tram
      targetSpeed = throttleSpeed;		  // set taget speed
      tramState = 10;
      tramTransition = 0;
    break;

   case 1011:                            // tram reached front end. stop it here
      targetSpeed = minSpeed;            // slow to stop
      tramState = 11;
      tramTransition = 0;
    break;
    
    case 1112:                                // stopped at front end. wait here
      timeToReStart = millis() + startDelay;  // set up station dwell time
      turnoutTarget = turnoutClosedPosition;  // throw turnout ready for other tram
      digitalWrite(rearRelayPin,    false);   // switch off power for this tram
      digitalWrite(throughRelayPin, true);    // power track for other tram
      colourSignalDanger(true);               // change singal to red
      tramState = 12;
      tramTransition = 0;
    break;

    case 121:                            // front end timer reached. start tram
      HCMotor.Direction(0, FORWARD);     // set diretion to forward
      targetSpeed = throttleSpeed;            // set taget speed
      tramState = 1;
      tramTransition = 0;
    break;
  }

/* check if obejctes are at their targets and call functions if they are not */

  if (turnoutPosition != turnoutTarget && millis() > turnoutMoveDelay) throwTurnout();
  if (signalPosition  != signalTarget  && millis() > signalThrowDelay) moveSignal();
  if (currentSpeed    != targetSpeed   && millis() > changeSpeedDelay) changeSpeed();
  
}
