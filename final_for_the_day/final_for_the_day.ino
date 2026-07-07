/*
  PIR Motion + Ultrasonic Distance Project
  ------------------------------------------
  Hardware: Arduino Uno, PIR sensor (HC-SR501), HC-SR04 ultrasonic,
            16x2 I2C LCD

  Wiring:
    PIR     -> VCC:5V  GND:GND  OUT:D2
    HC-SR04 -> VCC:5V  GND:GND  Trig:D9  Echo:D10
    I2C LCD -> VCC:5V  GND:GND  SDA:A4   SCL:A5

  Library needed: "LiquidCrystal I2C" by Frank de Brabander
  (install via Arduino IDE Library Manager)
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// If your LCD doesn't show text, try changing 0x27 to 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int PIR_PIN   = 2;
const int TRIG_PIN  = 9;
const int ECHO_PIN  = 10;

const int ALERT_DISTANCE = 15; // cm, distance considered "too close"

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Obstacle Sensor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(1500);
  lcd.clear();
}

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  if (duration == 0) return -1; // no echo received

  long distance = duration * 0.034 / 2; // convert to cm
  return distance;
}

// Tracks what was last shown, so we only touch the LCD/Serial
// when something actually changes.
int  lastMotion   = -1;   // -1 forces a print on the very first loop
long lastDistance = -999;

void loop() {
  int motion = digitalRead(PIR_PIN);
  long distance = -1;

  if (motion == HIGH) {
    distance = readDistanceCM();
  }

  // While motion is active, always refresh so the distance keeps
  // updating live. While idle, only print once when state changes
  // (so "No motion" doesn't spam the LCD/Serial every loop).
  bool motionChanged = (motion != lastMotion);
  bool shouldUpdate = (motion == HIGH) || motionChanged;

  if (shouldUpdate) {
    if (motion == HIGH) {
      lcd.setCursor(0, 0);
      lcd.print("Motion detected ");

      lcd.setCursor(0, 1);
      if (distance == -1) {
        lcd.print("Dist: out range ");
      } else if (distance <= ALERT_DISTANCE) {
        lcd.print("WARNING: close! ");
      } else {
        lcd.print("Dist: ");
        lcd.print(distance);
        lcd.print(" cm      ");
      }

      Serial.print("Motion: YES  Distance: ");
      Serial.print(distance);
      Serial.println(" cm");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("No motion       ");
      lcd.setCursor(0, 1);
      lcd.print("Waiting...      ");

      Serial.println("Motion: NO");
    }

    lastMotion = motion;
    lastDistance = distance;
  }

  delay(300);
}