#include "RobotDisplay.h"

RobotDisplay::RobotDisplay() : tft(TFT_eSPI()), spr(TFT_eSprite(&tft)) {
  eyeDist = 65;
  EVE_BLUE = tft.color565(0, 220, 255);

  EyeParams norm = { -0.15, 14.0, 16.0, 35.0, 0.0, 0.0 }; // \ / slightly tilted
  EyeParams blnk = {  0.00,  2.0,  2.0, 35.0, 0.0, 0.0 }; // - - horizontal lines
  EyeParams sad  = {  0.30, 16.0,  8.0, 35.0, 0.0, 0.0 }; // / \ inner big, outer small
  EyeParams hapy = { -0.15, 16.0, 16.0, 35.0, 0.0, 1.0 }; // \ / with crescent crop
  EyeParams angr = { -0.40, 16.0, 12.0, 35.0, 1.0, 0.0 }; // \ / steep with top crop

  states[0] = {norm, norm,  0.0}; // 0: Normaal
  states[1] = {blnk, blnk,  0.0}; // 1: Knipperen
  states[2] = {blnk, norm,  0.0}; // 2: Knipoog
  states[3] = {sad,  sad,   0.0}; // 3: Verdrietig 
  states[4] = {hapy, hapy,  0.0}; // 4: Blij 
  states[5] = {angr, angr,  0.0}; // 5: Boos (Midden)
  states[6] = {norm, norm, -1.0}; // 6: Links
  states[7] = {norm, norm,  1.0}; // 7: Rechts

  currentState = states[0];
  currentDrawnStateIndex = 0;

  lastAnim = 0;
  currentStep = 0;
  totalSteps = 11;

  int seq[] = {0, 1, 0, 1, 0, 7, 6, 7, 0, 2, 0};
  int dur[] = {5000, 150, 150, 150, 5000, 800, 800, 800, 5000, 300, 150};
  
  for(int i=0; i<11; i++) {
    animSequence[i] = seq[i];
    animDurations[i] = dur[i];
  }
}

void RobotDisplay::init() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK);
  spr.createSprite(SPRITE_W, SPRITE_H);

  drawEyesState(currentState, false);
}

EyeParams RobotDisplay::lerpParams(EyeParams a, EyeParams b, float t) {
  EyeParams res;
  res.tilt      = a.tilt + (b.tilt - a.tilt) * t;
  res.rInner    = a.rInner + (b.rInner - a.rInner) * t;
  res.rOuter    = a.rOuter + (b.rOuter - a.rOuter) * t;
  res.length    = a.length + (b.length - a.length) * t;
  res.cutTop    = a.cutTop + (b.cutTop - a.cutTop) * t;
  res.cutBottom = a.cutBottom + (b.cutBottom - a.cutBottom) * t;
  return res;
}

EyeState RobotDisplay::lerpState(EyeState a, EyeState b, float t) {
  EyeState res;
  res.left  = lerpParams(a.left, b.left, t);
  res.right = lerpParams(a.right, b.right, t);
  res.lookX = a.lookX + (b.lookX - a.lookX) * t;
  return res;
}

void RobotDisplay::drawEyesState(EyeState s, bool presenceDetected) {
  spr.fillSprite(TFT_BLACK);

  unsigned long now = millis();
  
  float floatY = sin(now / 800.0) * 2.0; 
  float wanderX = sin(now / 1000.0) * cos(now / 1500.0) * 1.5;

  int scx = (SPRITE_W / 2) + wanderX;
  int scy = (SPRITE_H / 2) + floatY;

  int lookOffset = s.lookX * 30; 
  int lx = scx - eyeDist + lookOffset;
  int ly = scy;
  int rx = scx + eyeDist + lookOffset;
  int ry = scy;

  // Draw Left Eye (\ indicates tilt down-right, meaning outer is high, inner is low: norm is slightly \ /)
  float pInL_x = lx + cos(s.left.tilt) * (s.left.length / 2.0);
  float pInL_y = ly - sin(s.left.tilt) * (s.left.length / 2.0);
  float pOutL_x = lx - cos(s.left.tilt) * (s.left.length / 2.0);
  float pOutL_y = ly + sin(s.left.tilt) * (s.left.length / 2.0);
  spr.drawWedgeLine(pInL_x, pInL_y, pOutL_x, pOutL_y, s.left.rInner, s.left.rOuter, EVE_BLUE);

  // Draw Right Eye
  float pInR_x = rx - cos(s.right.tilt) * (s.right.length / 2.0);
  float pInR_y = ry - sin(s.right.tilt) * (s.right.length / 2.0);
  float pOutR_x = rx + cos(s.right.tilt) * (s.right.length / 2.0);
  float pOutR_y = ry + sin(s.right.tilt) * (s.right.length / 2.0);
  spr.drawWedgeLine(pInR_x, pInR_y, pOutR_x, pOutR_y, s.right.rInner, s.right.rOuter, EVE_BLUE);

  // Cut Top (Angry flatten)
  if (s.left.cutTop > 0.01) {
    float dropL = 40.0 * s.left.cutTop; // 0 to 40 pixels down from the top boundary
    spr.fillRect(lx - 40, ly - 40, 80, dropL, TFT_BLACK);
  }
  if (s.right.cutTop > 0.01) {
    float dropR = 40.0 * s.right.cutTop; 
    spr.fillRect(rx - 40, ry - 40, 80, dropR, TFT_BLACK);
  }

  // Cut Bottom (Happy crescent)
  if (s.left.cutBottom > 0.01) {
    float offL = 35 - (s.left.cutBottom * 25);
    spr.fillEllipse(lx, ly + offL, 35, 25, TFT_BLACK);
  }
  if (s.right.cutBottom > 0.01) {
    float offR = 35 - (s.right.cutBottom * 25);
    spr.fillEllipse(rx, ry + offR, 35, 25, TFT_BLACK);
  }

  // --- PRESENCE INDICATOR ---
  if (presenceDetected) {
    spr.fillCircle(12, 12, 6, TFT_GREEN);
    spr.setTextColor(TFT_GREEN, TFT_BLACK);
    spr.drawString("Radar: ON", 24, 6, 2);
  } else {
    spr.fillCircle(12, 12, 6, TFT_RED);
    spr.setTextColor(TFT_RED, TFT_BLACK);
    spr.drawString("Radar: OFF", 24, 6, 2);
  }

  spr.pushSprite((tft.width() - SPRITE_W) / 2, (tft.height() - SPRITE_H) / 2);
}

void RobotDisplay::transitionTo(int targetIndex) {
  if (currentDrawnStateIndex == targetIndex) return;

  EyeState startState = currentState;
  EyeState targetState = states[targetIndex];
  int frames = (targetIndex == 1 || targetIndex == 2 || currentDrawnStateIndex == 1) ? 3 : 6; 

  // We temporarily hardcode presenceDetected to false during transition since it will be quickly redrawn in loop anyway
  for (int i = 1; i <= frames; i++) {
    float t = (float)i / frames; 
    currentState = lerpState(startState, targetState, t);
    drawEyesState(currentState, false);
  }
  
  currentDrawnStateIndex = targetIndex;
  currentState = targetState;
}

void RobotDisplay::update(unsigned long now, bool presenceDetected) {
  // Auto animation check
  if (now - lastAnim > animDurations[currentStep]) {
    currentStep++;
    if (currentStep >= totalSteps) currentStep = 0; 
    
    transitionTo(animSequence[currentStep]);
    lastAnim = now;
  }

  // Continuous draw for breathing/floating effect
  drawEyesState(currentState, presenceDetected);
}
