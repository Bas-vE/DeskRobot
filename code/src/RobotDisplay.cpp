#include "RobotDisplay.h"

RobotDisplay::RobotDisplay() : tft(TFT_eSPI()), spr(TFT_eSprite(&tft)) {
  eyeDist = 65;
  EVE_BLUE = tft.color565(0, 220, 255);

  states[0] = {0.0, 0.0, 0.0, 0.0, 0.0,  0.0}; // 0: Normaal
  states[1] = {0.9, 0.9, 0.0, 0.0, 0.0,  0.0}; // 1: Knipperen
  states[2] = {0.9, 0.0, 0.0, 0.0, 0.0,  0.0}; // 2: Knipoog
  states[3] = {0.0, 0.0, 0.0, 0.0, 1.0,  0.0}; // 3: Verdrietig 
  states[4] = {0.0, 0.0, 1.0, 0.0, 0.0,  0.0}; // 4: Blij 
  states[5] = {0.0, 0.0, 0.0, 1.0, 0.0,  0.0}; // 5: Boos (Midden)
  states[6] = {0.0, 0.0, 0.0, 0.0, 0.0, -1.0}; // 6: Links
  states[7] = {0.0, 0.0, 0.0, 0.0, 0.0,  1.0};  // 7: Rechts

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

EyeState RobotDisplay::lerpState(EyeState a, EyeState b, float t) {
  EyeState res;
  res.blinkL = a.blinkL + (b.blinkL - a.blinkL) * t;
  res.blinkR = a.blinkR + (b.blinkR - a.blinkR) * t;
  res.happy  = a.happy  + (b.happy  - a.happy)  * t;
  res.angry  = a.angry  + (b.angry  - a.angry)  * t;
  res.sad    = a.sad    + (b.sad    - a.sad)    * t;
  res.lookX  = a.lookX  + (b.lookX  - a.lookX)  * t;
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
  int eyeW = 70;
  int eyeHL = 100 - (s.blinkL * 90); 
  int eyeHR = 100 - (s.blinkR * 90);
  int radL = min(35, eyeHL / 2);
  int radR = min(35, eyeHR / 2);

  spr.fillSmoothRoundRect(lx - eyeW/2, ly - eyeHL/2, eyeW, eyeHL, radL, EVE_BLUE, TFT_BLACK);
  spr.fillSmoothRoundRect(rx - eyeW/2, ry - eyeHR/2, eyeW, eyeHR, radR, EVE_BLUE, TFT_BLACK);

  if (s.happy > 0.01) {
    int offsetY = 25 + ((1.0 - s.happy) * 50); 
    spr.fillEllipse(lx, ly + offsetY, 50, 40, TFT_BLACK);
    spr.fillEllipse(rx, ry + offsetY, 50, 40, TFT_BLACK);
  }
  if (s.angry > 0.01) {
    int dropY = -60 + (s.angry * 70); 
    spr.fillTriangle(lx - 15, ly - 60, lx + 55, ly - 60, lx + 55, ly + dropY, TFT_BLACK);
    spr.fillTriangle(rx + 15, ry - 60, rx - 55, ry - 60, rx - 55, ry + dropY, TFT_BLACK);
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
