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

enum RobotMode { MODE_NORMAL, MODE_HAPPY, MODE_SLEEPING };
RobotMode currentMode = MODE_NORMAL;

unsigned long modeStartTime = 0;
unsigned long happyDuration = 0;
unsigned long presenceStartTime = 0;
bool lastPresenceState = false;

void loop() {
  unsigned long now = millis();

  // 1. Read Sensor data
  bool presence = robotSensor.isPresenceDetected();
  
  // Presence/Absence detection logic for Sleep/Wake
  if (presence != lastPresenceState) {
    presenceStartTime = now;
    lastPresenceState = presence;
  }

  // State Transitions
  if (currentMode == MODE_SLEEPING) {
    if (presence && (now - presenceStartTime > 2000)) {
      currentMode = MODE_NORMAL;
      robotDisplay.setSleep(false);
      Serial.println("Waking up...");
    }
  } else {
    // If we lose presence, we go to sleep immediately
    if (!presence) {
      currentMode = MODE_SLEEPING;
      robotDisplay.setSleep(true);
      robotDisplay.setHappy(false);
      Serial.println("Going to sleep...");
    }
  }

  // Behavioral specific updates
  if (currentMode == MODE_NORMAL) {
    if (now - lastBlinkTime > nextBlinkInterval) {
      int roll = random(0, 10);
      if (roll < 3) {
        // Switch to Happy state
        currentMode = MODE_HAPPY;
        modeStartTime = now;
        happyDuration = random(15000, 20000); 
        robotDisplay.setHappy(true);
        Serial.println("Feeling happy...");
      } else {
        robotDisplay.blink(); 
      }
      lastBlinkTime = now;
      nextBlinkInterval = random(7000, 10000); 
    }
  } else if (currentMode == MODE_HAPPY) {
    if (now - modeStartTime > happyDuration) {
      currentMode = MODE_NORMAL;
      robotDisplay.setHappy(false);
      lastBlinkTime = now; // reset blink timer
      Serial.println("Back to normal.");
    }
  }

  // 2. Read Touch Input
  int touchStateTarget = robotTouch.checkTouch();
  if (touchStateTarget != -1) {
    robotDisplay.transitionTo(touchStateTarget);
  } else {
    // 3. Update Display
    robotDisplay.update(now, presence);
  }
}