#include "RobotDisplay.h"

RobotDisplay::RobotDisplay() : tft(TFT_eSPI()), spr(TFT_eSprite(&tft)) {
  eyeDist = 80;
  EVE_BLUE = tft.color565(0, 180, 255);
  EVE_BLUE_DARK = tft.color565(0, 40, 80);
  
  targetLookX = 0;
  currentLookX = 0;

  targetBlinkLeft = 0;
  currentBlinkLeft = 0;
  targetBlinkRight = 0;
  currentBlinkRight = 0;
}

void RobotDisplay::init() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK);
  
  // Set Sprite color depth to 8-bit to fit in ESP32 RAM (320x240x1 byte = 76.8KB)
  spr.setColorDepth(8);
  spr.createSprite(SPRITE_W, SPRITE_H);
}

void RobotDisplay::drawEveEye(int cx, int cy, bool isLeft, float blinkAmount) {
  // Eve's eyes tilt slightly backwards.
  // Left eye top points slightly inward/right (clockwise). Right eye top points left (counter-clockwise).
  float angle_deg = isLeft ? 15.0 : -15.0; 
  float rad = angle_deg * PI / 180.0;
  float cosA = cos(rad);
  float sinA = sin(rad);
  
  // Define bounding box for our math renderer to check pixels
  int w = 65; 
  int h = 50;
  
  // Base radii of the ellipse
  float a = 55.0; // horizontal 
  float b = 38.0; // vertical 
  
  // Draw pixel by pixel to form the precise shape with scanlines
  for (int y = -h; y <= h; y++) {
    for (int x = -w; x <= w; x++) {
      
      // Rotate coordinates to align with the titled eye
      float rx = x * cosA + y * sinA;
      float ry = -x * sinA + y * cosA;
      
      // Asymmetry factor: the inner side of Eve's eyes is thicker/rounder, the outer side is sharper.
      // Left eye: inner side is positive rx. Right eye: inner side is negative rx.
      float innerDir = isLeft ? 1.0 : -1.0;
      float x_ratio = rx / a; 
      
      // Scale taper effect
      float taper = 1.0 + 0.15 * (x_ratio * innerDir); 
      
      // Eve's eyes are slightly squashed/flat on the bottom
      // (y > 0 is bottom half within our local box coordinate)
      if (ry > 0) {
          taper *= 0.85; 
      }
      
      float ey_scaled = ry / taper;
      
      // Mathematical ellipse formula check
      if ((rx * rx) / (a * a) + (ey_scaled * ey_scaled) / (b * b) <= 1.0) {
        
        // Blink cutout logic (Dynamic crescent mask)
        // We carve a chunk out of the bottom using another wide ellipse.
        if (blinkAmount > 0.01) {
           float maskOffset_Y = 110.0 - (blinkAmount * 70.0); 
           float mask_a = 85.0; // wider than the eye
           float mask_b = 60.0; // tall enough to form a smooth curve
           float mask_ry = ry - maskOffset_Y; 
           
           if ((rx * rx) / (mask_a * mask_a) + (mask_ry * mask_ry) / (mask_b * mask_b) <= 1.0) {
               continue; // Pixel is carved out, do not draw!
           }
        }

        // Scanlines: every 3rd pixel line is drawn dark to mimic CRT pixels
        if ((cy + y) % 3 == 0) {
           spr.drawPixel(cx + x, cy + y, spr.color8to16(spr.color16to8(EVE_BLUE_DARK)));
        } else {
           spr.drawPixel(cx + x, cy + y, spr.color8to16(spr.color16to8(EVE_BLUE)));
        }
      }
    }
  }
}

void RobotDisplay::blink() {
  targetBlinkLeft = 1.0;
  targetBlinkRight = 1.0;
}

void RobotDisplay::wink(bool leftEye) {
  if (leftEye) {
    targetBlinkLeft = 1.0;
  } else {
    targetBlinkRight = 1.0;
  }
}

void RobotDisplay::transitionTo(int targetIndex) {
  // No complex animations anymore, just offset the look target
  if (targetIndex == 6) {       // Touch Left
    targetLookX = -1.0;
  } else if (targetIndex == 7) { // Touch Right
    targetLookX = 1.0;
  } else {                       // Touch Center / Auto
    targetLookX = 0.0;
  }
}

void RobotDisplay::update(unsigned long now, bool presenceDetected) {
  // Clear off-screen buffer
  spr.fillSprite(TFT_BLACK);
  
  // Smoothly lerp towards target look direction
  currentLookX = currentLookX + (targetLookX - currentLookX) * 0.1;

  // Smoothly lerp blink states (increased from 0.15 to 0.45 for much faster blinking)
  currentBlinkLeft = currentBlinkLeft + (targetBlinkLeft - currentBlinkLeft) * 0.45;
  currentBlinkRight = currentBlinkRight + (targetBlinkRight - currentBlinkRight) * 0.45;
  
  // Auto-reverse blink when almost fully closed (lowered threshold slightly from 0.95 to 0.90 for snappier return)
  if (targetBlinkLeft > 0.5 && currentBlinkLeft > 0.90) targetBlinkLeft = 0.0;
  if (targetBlinkRight > 0.5 && currentBlinkRight > 0.90) targetBlinkRight = 0.0;

  // Breathing / Hovering math
  float floatY = sin(now / 800.0) * 3.0; 
  float wanderX = sin(now / 1500.0) * cos(now / 2000.0) * 2.0;

  int scx = (SPRITE_W / 2) + wanderX;
  int scy = (SPRITE_H / 2) + floatY;

  // Add touch look offset
  int lookOffset = currentLookX * 25; 
  
  // Draw the two math-based static Eve eyes with dynamic blinking cutouts
  drawEveEye(scx - eyeDist + lookOffset, scy, true, currentBlinkLeft);
  drawEveEye(scx + eyeDist + lookOffset, scy, false, currentBlinkRight);

  // --- PRESENCE INDICATOR ---
  /*
  if (presenceDetected) {
    spr.fillCircle(12, 12, 6, spr.color8to16(spr.color16to8(TFT_GREEN)));
    spr.setTextColor(spr.color8to16(spr.color16to8(TFT_GREEN)), TFT_BLACK);
    spr.drawString("Radar: ON", 24, 6, 2);
  } else {
    spr.fillCircle(12, 12, 6, spr.color8to16(spr.color16to8(TFT_RED)));
    spr.setTextColor(spr.color8to16(spr.color16to8(TFT_RED)), TFT_BLACK);
    spr.drawString("Radar: OFF", 24, 6, 2);
  }
  */

  // Push sprite to TFT screen from center coordinate
  spr.pushSprite((tft.width() - SPRITE_W) / 2, (tft.height() - SPRITE_H) / 2);
}
