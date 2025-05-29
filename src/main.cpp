#include <TFT_eSPI.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// I2C pe D21/D22
#define SDA_PIN 21
#define SCL_PIN 22

#define US_TRIG 12
#define US_ECHO 13

TFT_eSPI      tft = TFT_eSPI();
Servo         myServo;
Adafruit_VL53L0X sensor;

void showMessage(const String& msg) {
  tft.fillRect(0, 0, tft.width(), 20, TFT_BLACK);
  tft.fillRect(0, 20, tft.width(), tft.height() - 20, TFT_NAVY);
  uint8_t old = tft.getTextDatum();
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(msg, tft.width() / 2, 2, 2);
  tft.setTextDatum(old);
}

void drawBarLevel(uint8_t lvl) {
  // Coordonate şi dimensiuni
  const int barX      = 20;
  const int barW      = tft.width() - 40;
  const int barH      = 20;
  const int barY      = tft.height() / 2 - barH / 2;
  const int segW      = barW / 3;

  // 1) Desenează cele 3 segmente cu culoarea potrivită
  for (uint8_t i = 0; i < 3; i++) {
    uint16_t col = (i < lvl) ? TFT_GREEN : TFT_DARKGREY;
    tft.fillRect(barX + i * segW, barY, segW, barH, col);
  }

  // 2) Trasează conturul
  tft.drawRect(barX, barY, barW, barH, TFT_WHITE);

  // 3) Afișează cifra sub bară, ştergând în spate cu NAVY
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);  
  tft.drawString(String(lvl), tft.width() / 2, barY + barH + 10, 4);

  // 4) Mută servo-ul
  int angle = map(lvl, 0, 3, 0, 180);
  //myServo.write(angle);
}


void setup() {
  // Serial debug
  Serial.begin(115200);
  while (!Serial);

  Serial.println("=== BOOT ===");

  // TFT
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 20, tft.width(), tft.height() - 20, TFT_NAVY);
  showMessage("Level");
  // Servo pe D14
  myServo.attach(14);
  myServo.write(-30);
  pinMode(US_TRIG, OUTPUT);
  pinMode(US_ECHO, INPUT);

  // I2C pe 21/22 cu pull-up intern și 400kHz
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  // Scan I2C pentru debugging
  Serial.println("Scanning I2C bus...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  Found 0x%02X\n", addr);
    }
  }

  const uint8_t TOF_ADDR = 0x30;
  if (! sensor.begin(TOF_ADDR)) {
    Serial.printf("VL53L0X failed to start at 0x%02X\n", TOF_ADDR);
  } else {
    Serial.printf("VL53L0X started OK at 0x%02X\n", TOF_ADDR);
  }

}

void loop() {
  VL53L0X_RangingMeasurementData_t meas;
  sensor.rangingTest(&meas, false);
  uint16_t dist = meas.RangeMilliMeter;

  Serial.printf("Distance: %d mm\n", dist);

  int lvl = map(dist, 30, 120, 3, 0);
  lvl = constrain(lvl, 0, 3);
  lvl = 3;
  if(dist > 170)
    lvl = 0;
  else if(dist <= 170 &&  dist > 120)
          lvl = 1;
          else if (dist <=120 && dist > 70)
            lvl = 2;
            else  lvl = 3;
  //showMessage("Level: " + String(lvl));
  drawBarLevel(lvl);
  digitalWrite(US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);

  // Măsoară durata răspunsului (în µs), timeout 30ms (~10m)
  long duration = pulseIn(US_ECHO, HIGH, 30000);
  float distUS = duration > 0
    ? (duration / 2.0f) * 0.0343f  // cm (speed = 343 m/s)
    : -1;

  if (distUS > 0) {
    //Serial.printf("US Distance: %.1f cm\n", distUS);
    // --- comandă servo pe baza US ---
    if (distUS < 15.0f) {
      myServo.write(90);
      delay(5000);
    } else {
      myServo.write(0);
    }  // pause să prindă poziţia
  } else {
    Serial.println("US: out of range");
  }
  
  delay(500);
}
