#ifndef ROBOTTOUCH_H
#define ROBOTTOUCH_H

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

class RobotTouch {
public:
  RobotTouch();
  void init();
  
  // Checks if touched, returning a target state index (5=angry, 6=left, 7=right)
  // Returns -1 if no touch happened.
  int checkTouch();

  bool isOverrideActive();
  void clearOverride();

private:
  XPT2046_Touchscreen ts;
  
  bool isTouchOverride;
  unsigned long lastTouchTime;
};

#endif
