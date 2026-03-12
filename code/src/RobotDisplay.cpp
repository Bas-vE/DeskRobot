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

  targetHappy = 0;
  currentHappyLeft = 0;
  currentHappyRight = 0;

  targetSleep = 0;
  currentSleep = 0;
  zFloatTime = 0;
  currentBrightness = 1.0;
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

void RobotDisplay::drawEveEye(int cx, int cy, bool isLeft, float blinkAmount, float happyAmount, float sleepAmount) {
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
  
  // Sleep squish: Eyes go to 0 height
  b = b * (1.0 - sleepAmount);

  // Squish the height towards the center for the blink animation (from current height down to 8 radius = 16px high)
  // If we are sleeping, b is already tiny or zero.
  if (b > 8.0) {
    b = b - (blinkAmount * (b - 8.0));
  }
  
  // If b is effectively zero, don't bother drawing
  if (b < 0.5) return;
  
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
        
        // Happy cutout logic (Dynamic crescent mask)
        // We carve a chunk out of the bottom using another wide ellipse.
        if (happyAmount > 0.01) {
           float maskOffset_Y = 125.0 - (happyAmount * 65.0); 
           float mask_a = 85.0; // wider than the eye
           float mask_b = 60.0; // tall enough to form a smooth curve
           float mask_ry = ry - maskOffset_Y; 
           
           if ((rx * rx) / (mask_a * mask_a) + (mask_ry * mask_ry) / (mask_b * mask_b) <= 1.0) {
               continue; // Pixel is carved out, do not draw!
           }
        }

        // Scanlines: every 3rd pixel line is drawn dark to mimic CRT pixels
        uint16_t baseColor = ((cy + y) % 3 == 0) ? EVE_BLUE_DARK : EVE_BLUE;
        
        if (currentBrightness < 0.99) {
          uint8_t r = (baseColor >> 11) & 0x1F;
          uint8_t g = (baseColor >> 5) & 0x3F;
          uint8_t b_low = baseColor & 0x1F;
          
          r = (uint8_t)(r * currentBrightness);
          g = (uint8_t)(g * currentBrightness);
          b_low = (uint8_t)(b_low * currentBrightness);
          
          uint16_t shadedColor = (r << 11) | (g << 5) | b_low;
          spr.drawPixel(cx + x, cy + y, shadedColor);
        } else {
          spr.drawPixel(cx + x, cy + y, baseColor);
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

void RobotDisplay::setHappy(bool active) {
  targetHappy = active ? 1.0 : 0.0;
}

void RobotDisplay::setSleep(bool active) {
  targetSleep = active ? 1.0 : 0.0;
}

void RobotDisplay::setBrightness(float b) {
  if (b < 0) b = 0;
  if (b > 1) b = 1;
  currentBrightness = b;
}

void RobotDisplay::drawZzz(int x, int y, int size, float offset) {
  // Draw a cohesive, math-based 'Z' with CRT scanlines
  // x, y is the center of the Z
  int curY = y - (offset * 60); // Drift upwards more
  int halfSize = size / 2;
  int thickness = size / 4;
  if (thickness < 4) thickness = 4;

  for (int dy = 0; dy < size; dy++) {
    for (int dx = 0; dx < size; dx++) {
      bool draw = false;
      
      // Top bar
      if (dy < thickness) draw = true;
      // Bottom bar
      else if (dy > size - thickness) draw = true;
      // Diagonal: connects top-right to bottom-left
      else {
        // Line x + y = size connects (size, 0) and (0, size)
        float dist = abs(dx + dy - size);
        if (dist < (thickness / 1.5f)) draw = true;
      }

      if (draw) {
        int sx = x + dx - halfSize;
        int sy = curY + dy - halfSize;
        
        // Bounds check
        if (sx < 0 || sx >= SPRITE_W || sy < 0 || sy >= SPRITE_H) continue;

        // CRT Scanline effect (consistent with eyes)
        uint16_t color = (sy % 3 == 0) ? EVE_BLUE_DARK : EVE_BLUE;
        
        // Apply global brightness (for fading/sleep)
        if (currentBrightness < 0.99) {
          uint8_t r = ((color >> 11) & 0x1F) * currentBrightness;
          uint8_t g = ((color >> 5) & 0x3F) * currentBrightness;
          uint8_t b = (color & 0x1F) * currentBrightness;
          color = (r << 11) | (g << 5) | b;
        }
        
        spr.drawPixel(sx, sy, color);
      }
    }
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

  // Smoothly lerp blink states
  currentBlinkLeft = currentBlinkLeft + (targetBlinkLeft - currentBlinkLeft) * 0.7;
  currentBlinkRight = currentBlinkRight + (targetBlinkRight - currentBlinkRight) * 0.7;
  
  // Smoothly lerp happy and sleep states (Increased happy speed to 0.5)
  currentHappyLeft = currentHappyLeft + (targetHappy - currentHappyLeft) * 0.7;
  currentHappyRight = currentHappyRight + (targetHappy - currentHappyRight) * 0.7;

  currentSleep = currentSleep + (targetSleep - currentSleep) * 0.15; // Increased from 0.05 to 0.15 for faster closing

  // Auto-reverse blink when almost fully closed
  if (targetBlinkLeft > 0.5 && currentBlinkLeft > 0.90) targetBlinkLeft = 0.0;
  if (targetBlinkRight > 0.5 && currentBlinkRight > 0.90) targetBlinkRight = 0.0;
  
  // Draw Z's if sleeping
  if (currentSleep > 0.9) {
    zFloatTime += 0.02;
    if (zFloatTime > 1.0) zFloatTime = 0.0;
    
    // 3 Z's with different sizes and phases - Centered
    drawZzz(SPRITE_W/2 - 50, SPRITE_H/2 + 30, 35, zFloatTime);
    drawZzz(SPRITE_W/2, SPRITE_H/2 + 10, 60, fmod(zFloatTime + 0.3, 1.0)); 
    drawZzz(SPRITE_W/2 + 60, SPRITE_H/2 + 20, 45, fmod(zFloatTime + 0.6, 1.0));
  }

  // Breathing / Hovering math
  float floatY = sin(now / 800.0) * 3.0; 
  float wanderX = sin(now / 1500.0) * cos(now / 2000.0) * 2.0;

  // Don't hover if sleeping
  if (currentSleep > 0.5) {
    floatY *= (1.0 - currentSleep);
    wanderX *= (1.0 - currentSleep);
  }

  int scx = (SPRITE_W / 2) + wanderX;
  int scy = (SPRITE_H / 2) + floatY;

  // Add touch look offset
  int lookOffset = currentLookX * 25; 
  
  // Draw the two math-based static Eve eyes with dynamic blinking and happy cutouts
  drawEveEye(scx - eyeDist + lookOffset, scy, true, currentBlinkLeft, currentHappyLeft, currentSleep);
  drawEveEye(scx + eyeDist + lookOffset, scy, false, currentBlinkRight, currentHappyRight, currentSleep);

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
