#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <FluxGarage_RoboEyes.h>

// ===== Pin mapping — matches J2 connector on your PCB (traced from schematic) =====
#define TFT_CS   D0   // GPIO16
#define TFT_RST  D5   // GPIO14
#define TFT_DC   D4   // GPIO2  (A0)
#define TFT_MOSI D1   // GPIO5  (SDA on J2)
#define TFT_SCK  D3   // GPIO0
// LED (backlight) is hardwired to 3V3 on the board — always on, no pin needed.

#define TOUCH    D2   // GPIO4 — free pin, wired here per your choice.
                       // NOTE: as of this schematic, SW1 (touch sensor) OUT only
                       // connects to CHARGER1 pin 3, not to the ESP8266. You'll
                       // need to run a jumper/trace from that net to D2 for this
                       // to actually read anything — the code alone can't fix that.

// This is a 128x160 panel; software SPI since none of these are hardware SPI pins.
Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

RoboEyes<Adafruit_ST7735> roboEyes(display);

static unsigned long touchedTime = 0;
bool moveNE = true;

void setup() {
  pinMode(TOUCH, INPUT);

  display.initR(INITR_BLACKTAB);   // most common 1.8" ST7735 variant; try INITR_GREENTAB if colors look off
  display.setRotation(0);
  display.fillScreen(ST77XX_BLACK);

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setPosition(DEFAULT);
  roboEyes.setMood(DEFAULT);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);

  touchedTime = millis();
}

void loop() {
  bool touched = digitalRead(TOUCH);
  unsigned long now = millis();

  // ========= TAP / TOUCH =========
  if (touched == HIGH) {

    roboEyes.setMood(HAPPY);
    roboEyes.setAutoblinker(OFF);
    roboEyes.setIdleMode(OFF);

    // smooth NE <-> NW movement
    if (now - touchedTime > 150) {
      touchedTime = now;

      if (moveNE) {
        roboEyes.setPosition(NE);
      } else {
        roboEyes.setPosition(NW);
      }
      moveNE = !moveNE;
    }
  }

  // ========= NORMAL IDLE =========
  else {
    roboEyes.setMood(DEFAULT);
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
    // DO NOT force setPosition(DEFAULT)
  }

  roboEyes.update();
}
