/*  Arduino Test Track
   A version of practial Arduino whihc controls 2 servos using millis() and arrays.
   The Arrays allow scaling up of the number from 2 servos to a maxium of 12 on an Uno or Nano
   Upto 48 servos can be controlled using a Mega
   Use NUMBER_OF_SERVOS to setup the number you have connected.
   servoPins[] holds the Pin number that each servo is connected to
   servoMin[] holds the closed angle for a turnout or the danger angle for a singal
   servoMax[] holds the thrown angle for a tirnout or the clear  angle for a signal
   Becuase the turnouts and signals are controlled by the same array there is no frog juicing functionallity.

*/

#include<Servo.h>

/* readable names for the I/O pins */
#define frontSensorPin  2
#define turnoutServoPin  6                //PWM pin
#define turnoutThrowPin 7
#define turnoutClosePin 8
#define signalServoPin 5           //PWM pin
#define divergeSensorPin 4
#define throughSensorPin 3
#define signalGreenPin 10          //PWM pin
#define signalRedPin   9           //PWM pin


// constants in caps

const byte NUMBER_OF_SERVOS = 2;
#define THROW_SPEED 30              // [ms] lower number is faster


/* assign servos in an array */
Servo allServos[NUMBER_OF_SERVOS];                                    // Define servo array

/* Setup servo arrays  */
int servoPins[NUMBER_OF_SERVOS] = {turnoutServoPin, signalServoPin};  // enter pin numbers. 1st number is for servos[0], 2nd for servos[1] etc
int servoTarget[NUMBER_OF_SERVOS] ;                                   // the angle you want the servo to turn to, will be set to either min or max
int servoPosition[NUMBER_OF_SERVOS] ;                                 // current angle of servo
unsigned long throwDelay[NUMBER_OF_SERVOS];

int servoMin[] = {50, 80};                                            // minimun throw angle. 1st number is for servos[0], 2nd for servos[1] etc.
int servoMax[] = {90, 150};                                           // maximum throw angle. 1st number is for servos[0], 2nd for servos[1] etc.


void throwServo(int i) {
 throwDelay[i] = millis() + THROW_SPEED;
 if (servoPosition[i] < servoTarget[i]) servoPosition[i]++;
 if (servoPosition[i] > servoTarget[i]) servoPosition[i]--;
 allServos[i].write(servoPosition[i]);
};


// Function to control the two aspect colour signal
void colourSignalDanger(bool danger) {
 digitalWrite(signalGreenPin, !danger);
 digitalWrite(signalRedPin, danger);
};


void setup() {
 Serial.begin(9600);

 pinMode(divergeSensorPin, INPUT);
 pinMode(throughSensorPin, INPUT);
 pinMode(frontSensorPin, INPUT);

 pinMode(turnoutThrowPin, INPUT_PULLUP);
 pinMode(turnoutClosePin, INPUT_PULLUP);

 pinMode(signalGreenPin, OUTPUT);
 pinMode(signalRedPin, OUTPUT);


 /* initailise servos and their position */

 for (int thisServo = 0; thisServo < NUMBER_OF_SERVOS; thisServo++) {
   servoTarget[thisServo] = servoMin[thisServo];           // setup targets as minimim positiion
   servoPosition[thisServo] = servoMin[thisServo];         // setup position to minimum throw
   throwDelay[thisServo] = 0;                              // setup throwDelays to zero so they are less than millis() on first use

   allServos[thisServo].attach(servoPins[thisServo]);
   allServos[thisServo].write(servoPosition[thisServo]);
 };


 /* initailise colour signal and frog positions */
 colourSignalDanger(true);
}



void loop() {
 /* reading sensors */
 if (digitalRead(frontSensorPin) == LOW) {
     servoTarget[1] = servoMin[1];
     colourSignalDanger(true);
 }
 if (digitalRead(throughSensorPin) == LOW) servoTarget[1] = servoMax[1];
 if (digitalRead(divergeSensorPin) == LOW) colourSignalDanger(false);


 /* reading push buttons */
 if (digitalRead(turnoutThrowPin) == LOW) servoTarget[0] = servoMax[0];    // push button to throw turnout (Max)
 if (digitalRead(turnoutClosePin) == LOW) servoTarget[0] = servoMin[0];    // push button to close turnout (Min)


 /* check all servos. Move a servo which isn't in position, but only if a delay has passed */

 for (int thisServo = 0; thisServo < NUMBER_OF_SERVOS; thisServo++) {    // loop through all of the servos
   if (servoPosition[thisServo] != servoTarget[thisServo]) {           // check that they are not at there taregt position
     if (millis() > throwDelay[thisServo]) {                           // check that enough delay has passed
       throwServo(thisServo);                                          // call throwServo to move the servo 1 degree towards its target
     }
   }
 }
}
