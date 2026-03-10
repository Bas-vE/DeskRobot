#include "RobotSensor.h"

RobotSensor::RobotSensor() : currentState(false) {
}

void RobotSensor::init() {
  pinMode(RADAR_PIN, INPUT_PULLDOWN);
}

bool RobotSensor::isPresenceDetected() {
  currentState = digitalRead(RADAR_PIN);
  return currentState;
}
