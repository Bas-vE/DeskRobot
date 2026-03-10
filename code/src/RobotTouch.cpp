#include "RobotTouch.h"

RobotTouch::RobotTouch() : ts(XPT2046_CS, XPT2046_IRQ), isTouchOverride(false), lastTouchTime(0) {
}

void RobotTouch::init() {
  SPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  ts.setRotation(1); 
}

int RobotTouch::checkTouch() {
  unsigned long now = millis();

  if (ts.touched()) {
    isTouchOverride = true;
    lastTouchTime = now;

    TS_Point p = ts.getPoint();
    int pixelX = map(p.x, 300, 3800, 0, 320);

    if (pixelX < 106) {
      return 6; // Links
    } else if (pixelX > 214) {
      return 7; // Rechts
    } else {
      return 5; // Midden = Boos
    }
  } 

  if (isTouchOverride && (now - lastTouchTime > 2500)) {
    isTouchOverride = false;
    return 0; // Return to normal state (0)
  }

  return -1; // No touch action needed
}

bool RobotTouch::isOverrideActive() {
  return isTouchOverride;
}

void RobotTouch::clearOverride() {
  isTouchOverride = false;
}
