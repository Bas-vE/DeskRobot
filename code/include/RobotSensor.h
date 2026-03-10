#ifndef ROBOTSENSOR_H
#define ROBOTSENSOR_H

#include <Arduino.h>

#define RADAR_PIN 22

class RobotSensor {
public:
  RobotSensor();
  void init();
  
  // Returns true if human presence is detected
  bool isPresenceDetected();

private:
  bool currentState;
};

#endif
