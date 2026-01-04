// pin connections
const int dirPin = 3; // direction pin
const int stepPin = 2; // step pin

void setup() {
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  // set direction of rotation to clockwise
  digitalWrite(dirPin, HIGH);
}

void loop() {
  // take one step
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(200);
  // pause before taking next step
  digitalWrite(stepPin, LOW);
  delayMicroseconds(200);
}