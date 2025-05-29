# Software Design – SmartCan (ESP32)

## 1. Mediu de dezvoltare
1. **Platformă:** ESP32 (model generic compatibil cu Arduino)  
2. **IDE / Toolchain:**  
   - **PlatformIO** (extensie VSCode) – recomandat pentru gestionarea facilă a dependențelor și configurațiilor de build  
   - **Alternativ:** Arduino IDE (versiunea ≥ 1.8.13) cu board-manager pentru ESP32  
3. **Limbaj:** C/C++ (standard Arduino)

---

## 2. Librării și surse 3rd-party

| Librărie             | Scop                                  | Observații                                      |
|----------------------|---------------------------------------|-------------------------------------------------|
| **TFT_eSPI**         | Controlul display-ului TFT            | Configurată pentru SPI; pinii definiți în User_Setup.h |
| **ESP32Servo**       | Generare semnal PWM pentru servo      | Suport ESP32 (evită conflict cu ledc)           |
| **Wire**             | Comunicație I²C                       | Folosește pinii D21/D22, pull-up intern, 400 kHz|
| **Adafruit_VL53L0X** | Senzor LIDAR Time-of-Flight (VL53L0X) | Wraper Adafruit; comunică prin I²C              |

> *Instalare:* toate librăriile sunt disponibile în PlatformIO Library Manager sau Arduino Library Manager.

---

## 3. Algoritmi și structuri planificate

1. **Citire senzori**  
   - **VL53L0X (LIDAR)**  
     - Inițializare la adresa `0x30`  
     - Citire cu `sensor.rangingTest(&meas, false)` → `meas.RangeMilliMeter`  
   - **Ultrasonic (US)**  
     - Trigger/Echo pe pinii `D12`/`D13`  
     - Timeout 30 ms (≈ 10 m), calcul distanță (cm):  
       ```cpp
       float distUS = (duration > 0)
           ? (duration/2.0f) * 0.0343f
           : -1;
       ```
2. **Mapare nivel [0…3]**  
   ```cpp
   if      (dist > 170)              lvl = 0;
   else if (dist <= 170 && dist > 120) lvl = 1;
   else if (dist <= 120 && dist >  70) lvl = 2;
   else                                lvl = 3;
3. Interfață grafică  
   - **Bară cu 3 segmente:**  
     ```cpp
     void drawBarLevel(uint8_t lvl) {
       const int barX = 20;
       const int barW = tft.width() - 40;
       const int barH = 20;
       const int barY = tft.height()/2 - barH/2;
       const int segW = barW / 3;

       // Desenare segmente
       for (uint8_t i = 0; i < 3; i++) {
         uint16_t col = (i < lvl) ? TFT_GREEN : TFT_DARKGREY;
         tft.fillRect(barX + i*segW, barY, segW, barH, col);
       }
       // Contur
       tft.drawRect(barX, barY, barW, barH, TFT_WHITE);

       // Text nivel
       tft.setTextDatum(TC_DATUM);
       tft.setTextColor(TFT_WHITE, TFT_NAVY);
       tft.drawString(String(lvl), tft.width()/2, barY + barH + 10, 4);
     }
     ```
   - **Mesaje header:**  
     ```cpp
     void showMessage(const String& msg) {
       tft.fillRect(0, 0, tft.width(), 20, TFT_BLACK);
       tft.fillRect(0, 20, tft.width(), tft.height()-20, TFT_NAVY);
       tft.setTextDatum(TC_DATUM);
       tft.setTextColor(TFT_WHITE);
       tft.drawString(msg, tft.width()/2, 2, 2);
     }
     ```

4. Control servo  
   - **Inițializare & poziție inițială:**  
     ```cpp
     void initServo() {
       myServo.attach(14);
       myServo.write(0);      // poziție de start
     }
     ```
   - **Unghi din nivel:**  
     ```cpp
     void setServoLevel(uint8_t lvl) {
       int angle = map(lvl, 0, 3, 0, 180);
       myServo.write(angle);
     }
     ```
   - **Mecanism de siguranță (US):**  
     ```cpp
     void safetyServo(float distUS) {
       if (distUS > 0 && distUS < 15.0f) {
         myServo.write(90);
         delay(5000);
       } else {
         myServo.write(0);
       }
     }
     ```

5. Etapa 3 – Surse și funcții implementate

| Modul / Fișier   | Funcții                 | Descriere                                |
|------------------|-------------------------|------------------------------------------|
| **main.cpp**     | `setup()`, `loop()`     | Inițializări și bucla principală         |
| **ui.hpp/.cpp**  | `showMessage()`, `drawBarLevel()` | Toate operațiunile grafice            |
| **sensors.hpp**  | `initSensors()`, `readTOF()`, `readUS()` | Init și citire senzori VL53L0X + US |
| **servo.hpp**    | `initServo()`, `setServoLevel()`, `safetyServo()` | Control servo și fallback US |

### Scurtă descriere a funcțiilor:
- **`initSensors()`**  
  Configurează I²C (21/22, pull-up, 400 kHz), inițializează VL53L0X (0x30).
- **`readTOF()`** → `uint16_t`  
  Rulează `sensor.rangingTest()` și returnează `RangeMilliMeter`.
- **`readUS()`** → `float`  
  Trimite trigger, citește `pulseIn()` și calculează cm sau -1.
- **`setServoLevel(uint8_t lvl)`**  
  Mapează nivel → unghi și scrie la servo.
- **`safetyServo(float distUS)`**  
  Detectează obstacol < 15 cm și rotește servo la 90° în fallback.

---


