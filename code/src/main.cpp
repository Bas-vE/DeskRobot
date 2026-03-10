#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

#define TFT_BL 21
#define RADAR_PIN 22
uint16_t EVE_BLUE = tft.color565(0, 220, 255);

#define SPRITE_W 240
#define SPRITE_H 160
int eyeDist = 65; 

struct EyeState {
  float blinkL; 
  float blinkR; 
  float happy;  
  float angry;  
  float sad;    
  float lookX;  
};

EyeState states[8] = {
  {0.0, 0.0, 0.0, 0.0, 0.0,  0.0}, // 0: Normaal
  {0.9, 0.9, 0.0, 0.0, 0.0,  0.0}, // 1: Knipperen
  {0.9, 0.0, 0.0, 0.0, 0.0,  0.0}, // 2: Knipoog
  {0.0, 0.0, 0.0, 0.0, 1.0,  0.0}, // 3: Verdrietig 
  {0.0, 0.0, 1.0, 0.0, 0.0,  0.0}, // 4: Blij 
  {0.0, 0.0, 0.0, 1.0, 0.0,  0.0}, // 5: Boos (Midden)
  {0.0, 0.0, 0.0, 0.0, 0.0, -1.0}, // 6: Links
  {0.0, 0.0, 0.0, 0.0, 0.0,  1.0}  // 7: Rechts
};

EyeState currentState = states[0];
int currentDrawnStateIndex = 0;

int animSequence[] = {0, 1, 0, 1, 0, 7, 6, 7, 0, 2, 0};
int animDurations[] = {5000, 150, 150, 150, 5000, 800, 800, 800, 5000, 300, 150};
int totalSteps = 11;

unsigned long lastAnim = 0;
int currentStep = 0;

bool isTouchOverride = false;
unsigned long lastTouchTime = 0;

bool isPresenceDetected = false;

EyeState lerpState(EyeState a, EyeState b, float t) {
  EyeState res;
  res.blinkL = a.blinkL + (b.blinkL - a.blinkL) * t;
  res.blinkR = a.blinkR + (b.blinkR - a.blinkR) * t;
  res.happy  = a.happy  + (b.happy  - a.happy)  * t;
  res.angry  = a.angry  + (b.angry  - a.angry)  * t;
  res.sad    = a.sad    + (b.sad    - a.sad)    * t;
  res.lookX  = a.lookX  + (b.lookX  - a.lookX)  * t;
  return res;
}

void drawEyesState(EyeState s) {
  spr.fillSprite(TFT_BLACK);

// --- LEVENDIGHEID (IDLE ANIMATION) ---
  unsigned long now = millis();
  
  // 1. Zweven (Floating): Kalm en ontspannen (delen door 800 = cyclus van ~5 seconden)
  // De * 2.0 zorgt dat ze maximaal 2 pixels omhoog en omlaag deint.
  float floatY = sin(now / 800.0) * 2.0; 
  
  // 2. Saccades (Zoeken): Trage, dromerige blikwisselingen
  float wanderX = sin(now / 1000.0) * cos(now / 1500.0) * 1.5;

  int scx = (SPRITE_W / 2) + wanderX;
  int scy = (SPRITE_H / 2) + floatY;
  // -------------------------------------

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
  if (isPresenceDetected) {
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

void transitionTo(int targetIndex) {
  if (currentDrawnStateIndex == targetIndex) return;

  EyeState startState = currentState;
  EyeState targetState = states[targetIndex];
  int frames = (targetIndex == 1 || targetIndex == 2 || currentDrawnStateIndex == 1) ? 3 : 6; 

  for (int i = 1; i <= frames; i++) {
    float t = (float)i / frames; 
    currentState = lerpState(startState, targetState, t);
    drawEyesState(currentState);
  }
  
  currentDrawnStateIndex = targetIndex;
  currentState = targetState;
}

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  pinMode(RADAR_PIN, INPUT);

  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK);
  spr.createSprite(SPRITE_W, SPRITE_H);

  SPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  ts.setRotation(1); 

  drawEyesState(currentState);
}

void loop() {
  unsigned long now = millis();

  isPresenceDetected = digitalRead(RADAR_PIN);

  if (ts.touched()) {
    isTouchOverride = true;
    lastTouchTime = now;

    TS_Point p = ts.getPoint();
    int pixelX = map(p.x, 300, 3800, 0, 320);

    if (pixelX < 106) {
      transitionTo(6); // Links
    } else if (pixelX > 214) {
      transitionTo(7); // Rechts
    } else {
      transitionTo(5); // Midden = Boos
    }
  } 
  else {
    if (isTouchOverride && (now - lastTouchTime > 2500)) {
      isTouchOverride = false; 
      lastAnim = now;          
      transitionTo(0); 
    }

    if (!isTouchOverride) {
      if (now - lastAnim > animDurations[currentStep]) {
        currentStep++;
        if (currentStep >= totalSteps) currentStep = 0; 
        
        transitionTo(animSequence[currentStep]);
        lastAnim = now;
      }
    }
  }

  // --- CONTINU TEKENEN ---
  // Dit is de magie! Zelfs als er geen grote animatie afspeelt,
  // updaten we het scherm continu met de nieuwe 'zweef' coördinaten.
  drawEyesState(currentState);
}