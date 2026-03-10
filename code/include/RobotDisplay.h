#ifndef ROBOTDISPLAY_H
#define ROBOTDISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>

#define SPRITE_W 240
#define SPRITE_H 160
#define TFT_BL 21

class RobotDisplay {
public:
  RobotDisplay();
  void init();
  
  // Update the eye positions dynamically
  void update(unsigned long now, bool presenceDetected);
  
  // 6 = links, 7 = rechts, any other = center/normal
  void transitionTo(int targetIndex); 

private:
  TFT_eSPI tft;
  TFT_eSprite spr;

  int eyeDist;
  uint16_t EVE_BLUE;
  uint16_t EVE_BLUE_DARK;

  float targetLookX;
  float currentLookX;

  // Draws a math-based tilted oval mimicking Eve's eyes
  void drawEveEye(int cx, int cy, bool isLeft);
};

#endif
