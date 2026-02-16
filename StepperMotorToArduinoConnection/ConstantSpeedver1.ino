#include <AccelStepper.h>

// Pin connections
const int dirPin = 19; 
const int stepPin = 18; 

// Initialize with (Type:Driver, Step, Dir)
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);

void setup() {

stepper.setMaxSpeed(2000); // Much slower

stepper.setSpeed(1000); // 500 pulses per second

}
void loop()
{  
  // runSpeed() keeps the motor spinning at the setSpeed() indefinitely
  stepper.runSpeed();
}