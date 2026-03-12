#include <Arduino.h>
#include "RobotDisplay.h"
#include "RobotTouch.h"
#include "RobotSensor.h"

// Instantiate our core modules
RobotDisplay robotDisplay;
RobotTouch robotTouch;
RobotSensor robotSensor;

enum RobotMode { MODE_NORMAL, MODE_HAPPY, MODE_SLEEPING, MODE_WAKING, MODE_INTERACT };
RobotMode currentMode = MODE_WAKING;

unsigned long lastBlinkTime = 0;
int nextBlinkInterval = 3000;
unsigned long wakingStartTime = 0;
int wakingStep = 0;
unsigned long modeStartTime = 0;
int interactStep = 0;
unsigned long happyDuration = 0;
unsigned long presenceStartTime = 0;
bool lastPresenceState = false;

void setup() {
  Serial.begin(115200);

  // Initialize all hardware modules
  robotSensor.init();
  robotDisplay.init();
  robotTouch.init();

  wakingStartTime = millis();
  robotDisplay.setBrightness(0.0);

  Serial.println("EVE-TV Modules Initialized - Starting Wake Sequence");
}

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
    if (presence && (now - presenceStartTime > 5000)) { // 5 second wake-up delay
      currentMode = MODE_WAKING;
      wakingStartTime = now;
      wakingStep = 0;
      robotDisplay.setSleep(false);
      robotDisplay.setBrightness(0.0);
      Serial.println("Starting wake sequence...");
    }
  } else if (currentMode == MODE_WAKING) {
    unsigned long elapsed = now - wakingStartTime;
    
    // Stage 1: Fade in (0-0.8s)
    if (elapsed < 800) {
      float b = (float)elapsed / 800.0f;
      robotDisplay.setBrightness(b);
    } else if (elapsed < 2800) {
      // Stage 2: Blinking (0.8-2.8s)
      robotDisplay.setBrightness(1.0);
      
      // Fast blinks every 400ms
      if (((elapsed - 800) / 400) > wakingStep) {
        robotDisplay.blink();
        wakingStep++;
      }
    } else {
      // Sequence complete
      currentMode = MODE_NORMAL;
      robotDisplay.setBrightness(1.0);
      lastBlinkTime = now;
      Serial.println("Back to normal.");
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
  } else if (currentMode == MODE_INTERACT) {
    unsigned long elapsed = now - modeStartTime;
    
    // Step 0: Eyes squint into Happy (0 - 0.4s)
    if (interactStep == 0) {
      robotDisplay.setHappy(true);
      if (elapsed > 400) {
        interactStep = 1;
        modeStartTime = now;
        robotDisplay.setBlush(true); // Start fading in blush
        Serial.println("Interacting: Blush appearing...");
      }
    } 
    // Step 1: Hold Blush (Heart warmed) (0 - 5.0s)
    else if (interactStep == 1) {
      if (elapsed > 5000) {
        interactStep = 2;
        modeStartTime = now;
        robotDisplay.setBlush(false); // Start fading out blush
        Serial.println("Interacting: Blush fading out...");
      }
    }
    // Step 2: Final Graceful Fade out of Eyes (Wait for blush to clear first)
    else if (interactStep == 2) {
      if (elapsed > 600) { // Give blush time to fade out (faster now)
        robotDisplay.setHappy(false); // Start graceful eye return
        currentMode = MODE_NORMAL;
        lastBlinkTime = now;
        Serial.println("Interacting: Eyes returning to normal. Heart warmed.");
      }
    }
  }

  // 2. Read Touch Input
  int touchStateTarget = robotTouch.checkTouch();
  if (touchStateTarget >= 0) {
    // If touch detected (5, 6, 7 or return-to-zero 0)
    // We only trigger interaction on actual presses (5, 6, 7)
    if (touchStateTarget >= 5 && currentMode != MODE_SLEEPING && currentMode != MODE_WAKING) {
      currentMode = MODE_INTERACT;
      interactStep = 0;
      modeStartTime = now;
      robotDisplay.setHappy(true); // Ensure eyes start turning happy immediately
    }
    robotDisplay.transitionTo(touchStateTarget);
  } else {
    // 3. Update Display
    robotDisplay.update(now, presence);
  }
}