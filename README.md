# EVE / WALL-E Desktop Companion Robot

![Concept Enclosure](images/concept.png)

An interactive desktop robot based on the **ESP32-2432S028R** (also known as the *Cheap Yellow Display* or CYD). This project combines the charm of WALL-E (the caterpillar tracks and retro look) with the expressive, digitally animated eyes of EVE.

The robot looks "alive" with smooth animations, responds to touches via the touchscreen, and will in the future automatically wake up when someone sits down at the desk.

## ✨ Features
- **Fluid Animations (Tweening):** Frame-by-frame transitions between emotions with anti-aliasing (smooth edges) via the TFT_eSprite engine.
- **Idle "Breathing" Animation:** The eyes continuously float slightly and make micro-movements (saccades), making the robot feel truly alive even when standing still.
- **Touch Interaction:** - Tap left = Looks left
  - Tap right = Looks right
  - Tap center = Looks angry
- **Automated Script:** Independently plays a sequence of glances and winks when left alone.

## 🛠 Hardware Requirements
* **Microcontroller/Screen:** ESP32-2432S028R (CYD with Resistive Touch).
* **Enclosure:** Custom 3D-printed retro TV enclosure on caterpillar tracks (in development).
* *(Planned)* **Presence Sensor:** HLK-LD2410 mmWave Radar to detect through the enclosure's plastic whether someone is sitting at the desk.

## 💻 Software & Dependencies
This project is built with **PlatformIO** (VS Code). 
It uses the following libraries:
* `bodmer/TFT_eSPI` (For lightning-fast screen rendering)
* `paulstoffregen/XPT2046_Touchscreen` (For touch input)

### The "CYD Touch" Fix
*Note:* This specific board has the display and the touch chip on **two separate SPI buses**. The code solves this by forcing the screen to the HSPI bus via the `platformio.ini` build flags, while the touch chip uses the standard VSPI bus. 

## 🚀 Installation
1. Clone this repository to your local machine.
2. Open the folder in **VS Code** with the **PlatformIO** extension installed.
3. Ensure your `platformio.ini` has the correct settings (see configuration below).
4. Click **Build** and then **Upload**.

### Important `platformio.ini` settings:
Ensure these build flags are in your configuration to prevent a flashing white screen or a non-working touchscreen:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    bodmer/TFT_eSPI
    paulstoffregen/XPT2046_Touchscreen

build_flags =
    -D USER_SETUP_LOADED=1
    -D ILI9341_2_DRIVER=1
    -D USE_HSPI_PORT=1       ; Force the screen to use the 2nd SPI bus
    -D TFT_WIDTH=240
    -D TFT_HEIGHT=320
    -D TFT_MISO=12
    -D TFT_MOSI=13
    -D TFT_SCLK=14
    -D TFT_CS=15
    -D TFT_DC=2
    -D TFT_RST=-1
    -D TFT_BL=21
    -D TOUCH_CS=33
    -D LOAD_GLCD=1
    -D LOAD_FONT2=1
    -D LOAD_FONT4=1
    -D SPI_FREQUENCY=55000000