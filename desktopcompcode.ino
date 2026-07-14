#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <FluxGarage_RoboEyes.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ==== TFT pins — update to match your wiring ====
#define TFT_CS   D0
#define TFT_RST  D5
#define TFT_DC   D4
#define TFT_MOSI D1
#define TFT_SCK  D3

#define TOUCH D2   // update if this collides with your I2C pins

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);
RoboEyes<Adafruit_ST7735> roboEyes(display);
Adafruit_MPU6050 mpu;

// time
unsigned long now;
unsigned long touchedTime = 0;
unsigned long moodStartTime = 0;
unsigned long cooldownUntil = 0;

// states
bool angryLowActive  = false;
bool angryHighActive = false;
bool moveNE = true;

// mpu data
float lastAx = 0, lastAy = 0, lastAz = 0;
float lowShakeThreshold  = 0.7;
float highShakeThreshold = 2.0;

// High Shake
unsigned long highShakeWindowStart = 0;
int highShakeHits = 0;

const int HIGH_SHAKE_HITS_REQUIRED = 3;
const unsigned long HIGH_SHAKE_WINDOW = 200;

// durations
const unsigned long LOW_SHAKE_TIME  = 2000;
const unsigned long HIGH_SHAKE_TIME = 3000;

// cool down
const unsigned long LOW_SHAKE_COOLDOWN  = 500;
const unsigned long HIGH_SHAKE_COOLDOWN = 1000;

// centered text (ST7735 draws directly, no display.display() needed)
void showCenteredText(const char* text) {
  display.fillScreen(ST77XX_BLACK);
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int x = (SCREEN_WIDTH - w) / 2;
  int y = (SCREEN_HEIGHT - h) / 2;

  display.setCursor(x, y);
  display.println(text);
}

void setup() {
  pinMode(TOUCH, INPUT);
  Wire.begin(D2, D1);   // MPU6050 I2C — update pins if they clash with TFT above
  Serial.begin(115200);

  display.initR(INITR_BLACKTAB);   // try INITR_GREENTAB if colors/offset look wrong
  display.setRotation(0);

  showCenteredText("WELCOME");
  delay(1200);
  showCenteredText("HI");
  delay(1000);

  display.fillScreen(ST77XX_BLACK);
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(8, 60);
  display.println("I AM YOUR");
  display.setCursor(14, 78);
  display.println("COMPANION");
  delay(1500);

  display.fillScreen(ST77XX_BLACK);

  if (!mpu.begin(0x68)) {
    while (true);
  }

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setMood(DEFAULT);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  lastAx = a.acceleration.x;
  lastAy = a.acceleration.y;
  lastAz = a.acceleration.z;
}

void loop() {
  now = millis();

  // happy
  if (digitalRead(TOUCH) == HIGH) {
    angryLowActive = false;
    angryHighActive = false;
    highShakeHits = 0;
    highShakeWindowStart = 0;

    roboEyes.setMood(HAPPY);
    roboEyes.setAutoblinker(OFF);
    roboEyes.setIdleMode(OFF);

    if (now - touchedTime > 150) {
      touchedTime = now;
      roboEyes.setPosition(moveNE ? NE : NW);
      moveNE = !moveNE;
    }

    roboEyes.update();
    return;
  }

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  float dx = a.acceleration.x - lastAx;
  float dy = a.acceleration.y - lastAy;
  float dz = a.acceleration.z - lastAz;

  float movement = sqrt(dx * dx + dy * dy + dz * dz);

  lastAx = a.acceleration.x;
  lastAy = a.acceleration.y;
  lastAz = a.acceleration.z;

  // shake logic
  if (now >= cooldownUntil) {

    if (movement > highShakeThreshold) {
      if (highShakeWindowStart == 0) {
        highShakeWindowStart = now;
        highShakeHits = 1;
      } else {
        highShakeHits++;
      }

      if (highShakeHits >= HIGH_SHAKE_HITS_REQUIRED &&
          now - highShakeWindowStart <= HIGH_SHAKE_WINDOW) {

        angryHighActive = true;
        angryLowActive = false;
        moodStartTime = now;
        cooldownUntil = now + HIGH_SHAKE_COOLDOWN;

        highShakeHits = 0;
        highShakeWindowStart = 0;
      }
    }
    else if (movement > lowShakeThreshold) {
      angryLowActive = true;
      moodStartTime = now;
      cooldownUntil = now + LOW_SHAKE_COOLDOWN;
    }
    else {
      highShakeHits = 0;
      highShakeWindowStart = 0;
    }
  }

  if (angryHighActive) {
    roboEyes.setMood(ANGRY);
    roboEyes.setHFlicker(ON, 2);
    roboEyes.setVFlicker(ON, 2);
    roboEyes.setAutoblinker(ON, 2, 2);
    roboEyes.setIdleMode(ON, 1, 1);

    if (now - moodStartTime >= HIGH_SHAKE_TIME) {
      angryHighActive = false;
      roboEyes.setHFlicker(OFF);
      roboEyes.setVFlicker(OFF);
    }
  }
  else if (angryLowActive) {
    roboEyes.setMood(ANGRY);
    roboEyes.setAutoblinker(ON, 2, 1);
    roboEyes.setIdleMode(OFF);

    if (now - moodStartTime >= LOW_SHAKE_TIME) {
      angryLowActive = false;
    }
  }
  else {
    roboEyes.setMood(DEFAULT);
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
  }

  roboEyes.update();
}