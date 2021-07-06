#include<Servo.h>


// constants in caps

const byte NUMBER_OF_PUSH_BUTTONS = 4;
const byte NUMBER_OF_ROUTE_BUTTONS = 2;
const byte NUMBER_OF_TURNOUTS = 5;
#define TURNOUT_MOVE_SPEED 30              // [ms] lower number is faster


/* assign servos in an array */
Servo turnoutServo[NUMBER_OF_TURNOUTS];                     // Define servo array

/* Setup servo arrays  */
int pushButtonPins[NUMBER_OF_PUSH_BUTTONS] = {7, 8, 9, 10}; // enter pin numbers. 1st number is for pushButtonPins[0], 2nd for pushButtonPins[1] etc
int routeButtonPins[NUMBER_OF_ROUTE_BUTTONS] = {11, 12};    // enter pin numbers. 1st number is for routeButtonPins[0], 2nd for routeButtonPins[1] etc
int turnoutPins[NUMBER_OF_TURNOUTS] = {2, 3, 4, 5, 6};      // eWnter pin numbers. 1st number is for allTurnouts[0], 2nd for allTurnouts[1] etc
int turnoutTarget[NUMBER_OF_TURNOUTS] ;                     // the angle you want the servo to turn to, will be set to either min or max
int turnoutPosition[NUMBER_OF_TURNOUTS] ;                   // current angle of servo
unsigned long turnoutMoveDelay[NUMBER_OF_TURNOUTS];         // valuse to compare with millis() to check if it is time to move a turnoutServo

int turnoutClosedPosition[] = {50, 80,  60,  60,  60};      // Turnout closed angle. 1st number is for allTurnouts[0], 2nd for allTurnouts[1] etc.
int turnoutThrownPosition[] = {90, 150, 120, 120, 120};     // Turnout thrown angle. 1st number is for allTurnouts[0], 2nd for allTurnouts[1] etc.
bool turnoutIsClosed[NUMBER_OF_TURNOUTS];                   // turnout state

void throwTurnout(int i){
  turnoutMoveDelay[i] = millis() + TURNOUT_MOVE_SPEED;
  if (turnoutPosition[i] < turnoutTarget[i]) turnoutPosition[i]++;
  if (turnoutPosition[i] > turnoutTarget[i]) turnoutPosition[i]--;
  turnoutServo[i].write(turnoutPosition[i]);
  if (turnoutPosition[i] == turnoutClosedPosition[i]) {
    turnoutIsClosed[i] = true;
    if (i == 3) turnoutTarget[i+1] = turnoutClosedPosition[i+1];
  }
  if (turnoutPosition[i] == turnoutThrownPosition[i]) {
    turnoutIsClosed[i] = false;
    if (i == 3) turnoutTarget[i+1] = turnoutThrownPosition[i+1];
  }
}

void setup() {
  Serial.begin(9600);
  /* setup all the servos*/
  for (int thisTurnout = 0; thisTurnout < NUMBER_OF_TURNOUTS; thisTurnout++) {
      turnoutTarget[thisTurnout]   = turnoutClosedPosition[thisTurnout];      // setup targets as closed positiion
      turnoutPosition[thisTurnout] = turnoutClosedPosition[thisTurnout];      // setup position to closed
      turnoutIsClosed[thisTurnout] = true;                                    // setup closed is true
      
      turnoutServo[thisTurnout].attach(turnoutPins[thisTurnout]);             // attach all the turnout servos
      turnoutServo[thisTurnout].write(turnoutPosition[thisTurnout]);          // write the servos poistions
  }
  
  /* setup all the pushbuttons */
  for (int i = 0; i < NUMBER_OF_PUSH_BUTTONS;  i++) pinMode(pushButtonPins[i], INPUT_PULLUP);      // initailise all the push buttons with pullups 
  for (int i = 0; i < NUMBER_OF_ROUTE_BUTTONS; i++) pinMode(routeButtonPins[i], INPUT_PULLUP);     // initailise all the route buttons with pullups 

}

void loop() {
  /* loop through all the push buttons */
  for (int i = 0; i < NUMBER_OF_PUSH_BUTTONS; i++){
    if (digitalRead(pushButtonPins[i]) == LOW){                               // is push button pressed
      if (turnoutIsClosed[i])  turnoutTarget[i] = turnoutThrownPosition[i];   // set turnout target to thrown if it is closed
      if (!turnoutIsClosed[i]) turnoutTarget[i] = turnoutClosedPosition[i];   // set turnout target to closed if it is thrown
    }
  }
  
  /* read route buttons and move turnouts when pressed */
  if (digitalRead(routeButtonPins[0]) == LOW){                              // is route button pressed
    if (!turnoutIsClosed[0]) turnoutTarget[0] = turnoutClosedPosition[0];   // set turnout target to closed if it is thrown
    if (!turnoutIsClosed[1]) turnoutTarget[1] = turnoutClosedPosition[1];   // set turnout target to closed if it is thrown
  }
  if (digitalRead(routeButtonPins[1]) == LOW){                           // is route button pressed
    if (turnoutIsClosed[0]) turnoutTarget[0] = turnoutThrownPosition[0];   // set turnout target to thrown if it is thrown
    if (turnoutIsClosed[1]) turnoutTarget[1] = turnoutThrownPosition[1];   // set turnout target to thrown if it is thrown
  }

  
  /* loop through the turnout servos */
  for (int thisTurnout = 0; thisTurnout < NUMBER_OF_TURNOUTS; thisTurnout++) {
    if (turnoutPosition[thisTurnout] != turnoutTarget[thisTurnout]) {           // check that they are not at there taregt position
      if (millis() > turnoutMoveDelay[thisTurnout]) {                           // check that enough delay has passed
           throwTurnout(thisTurnout);                                          // call throwTurnout to move the servo 1 degree towards its target
      }
    }
  }
}
