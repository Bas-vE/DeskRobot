#include <Arduino.h>
#include "RobotDisplay.h"
#include "RobotTouch.h"
#include "RobotSensor.h"

// Instantiate our core modules
RobotDisplay robotDisplay;
RobotTouch robotTouch;
RobotSensor robotSensor;

void setup() {
  Serial.begin(115200);

  // Initialize all hardware modules
  robotSensor.init();
  robotDisplay.init();
  robotTouch.init();

  Serial.println("EVE-TV Modules Initialized");
}

unsigned long lastBlinkTime = 0;
int nextBlinkInterval = 3000;

void loop() {
  unsigned long now = millis();

  // 1. Read Sensor data
  bool presence = robotSensor.isPresenceDetected();

  // 2. Read Touch Input
  int touchStateTarget = robotTouch.checkTouch();
  if (touchStateTarget != -1) {
    // If a touch event requires an animation change
    robotDisplay.transitionTo(touchStateTarget);
  } else if (!robotTouch.isOverrideActive()) {
    // 3. Update AI/Animations (Only if we aren't being overridden by touch)
    if (now - lastBlinkTime > nextBlinkInterval) {
      int roll = random(0, 10);
      if (roll < 2) {
        robotDisplay.wink(random(0, 2) == 0); // 20% chance to randomly wink
      } else if (roll < 4) {
        robotDisplay.triggerHappy();          // 20% chance to do a Happy crescent expression
      } else {
        robotDisplay.blink();                 // 60% chance to do a normal blink
      }
      lastBlinkTime = now;
      nextBlinkInterval = random(7000, 10000); // Wait 2 to 6 seconds for the next blink
    }
    
    robotDisplay.update(now, presence);
  } else {
    // If touch override is active but we received -1, it means we are in the timeout period. 
    // Simply render the current state.
    robotDisplay.update(now, presence); // (The update loop handles its own internal timing)
  }
}