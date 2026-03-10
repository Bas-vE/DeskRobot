#ifndef ROBOTDISPLAY_H
#define ROBOTDISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>

#define SPRITE_W 240
#define SPRITE_H 160
#define TFT_BL 21

struct EyeState {
  float blinkL; 
  float blinkR; 
  float happy;  
  float angry;  
  float sad;    
  float lookX;  
};

class RobotDisplay {
public:
  RobotDisplay();
  void init();
  void update(unsigned long now, bool presenceDetected);
  void transitionTo(int targetIndex);

private:
  TFT_eSPI tft;
  TFT_eSprite spr;

  int eyeDist;
  uint16_t EVE_BLUE;

  EyeState states[8];
  EyeState currentState;
  int currentDrawnStateIndex;

  unsigned long lastAnim;
  int currentStep;
  int totalSteps;
  int animSequence[11];
  int animDurations[11];

  EyeState lerpState(EyeState a, EyeState b, float t);
  void drawEyesState(EyeState s, bool presenceDetected);
};

#endif
