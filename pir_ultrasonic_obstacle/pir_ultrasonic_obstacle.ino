# PIR Motion + Ultrasonic Distance Detection System

An Arduino-based project that combines a **PIR motion sensor**, an **HC-SR04 ultrasonic distance sensor**, and a **16x2 I2C LCD** to detect motion and display live distance readings, with a proximity warning system.

## Hardware Used

| Component            | Pin Connections                     |
|-----------------------|--------------------------------------|
| Arduino Uno            | —                                    |
| PIR Sensor (HC-SR501)  | VCC: 5V, GND: GND, OUT: D2           |
| HC-SR04 Ultrasonic     | VCC: 5V, GND: GND, Trig: D9, Echo: D10 |
| 16x2 I2C LCD           | VCC: 5V, GND: GND, SDA: A4, SCL: A5  |

## Required Library

- **LiquidCrystal I2C** by Frank de Brabander — install via Arduino IDE: `Sketch > Include Library > Manage Libraries`, then search "LiquidCrystal I2C."

> **Note:** If the LCD doesn't display text, try changing the I2C address in the code from `0x27` to `0x3F`. This varies by LCD backpack manufacturer.

## How It Works

1. The PIR sensor continuously checks for motion.
2. When motion is detected, the HC-SR04 ultrasonic sensor measures distance to the nearest object.
3. The 16x2 LCD displays real-time status:
   - **No motion** → "No motion / Waiting..."
   - **Motion detected + object far away** → "Motion detected / Dist: XX cm"
   - **Motion detected + object very close (≤15 cm)** → "Motion detected / WARNING: close!"
   - **Motion detected + no valid echo** → "Motion detected / Dist: out range"
4. The Arduino avoids re-printing identical messages every loop, refreshing only when something actually changes (or continuously while motion is active, to keep distance live).

## Full Code

```arduino
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
'
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
'
```

## Code Walkthrough

### 1. Includes and LCD Setup

```arduino
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
```

- `Wire.h` enables I2C communication (required for the LCD, which uses SDA/SCL rather than individual data pins).
- `LiquidCrystal_I2C.h` is the driver library for the 16x2 I2C LCD.
- `lcd(0x27, 16, 2)` creates the LCD object at I2C address `0x27`, sized 16 columns × 2 rows.

### 2. Pin and Constant Definitions

```arduino
const int PIR_PIN = 2;
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;
const int ALERT_DISTANCE = 15;
```

Named constants make the code self-documenting and easy to modify if rewired. `ALERT_DISTANCE` sets the "too close" threshold in centimeters.

### 3. `setup()` — Runs Once at Startup

- Configures pin modes: PIR and Echo as `INPUT` (they send data to the Arduino), Trig as `OUTPUT` (Arduino sends a pulse out).
- Starts Serial communication at 9600 baud for debug output.
- Initializes the LCD, turns on the backlight, and shows a 1.5-second "Obstacle Sensor / Initializing..." splash screen before clearing it.

### 4. `readDistanceCM()` — Ultrasonic Distance Measurement

Standard HC-SR04 ping sequence:

1. Ensures Trig starts LOW, then briefly pulses it HIGH for 10 microseconds to trigger an ultrasonic "chirp."
2. `pulseIn(ECHO_PIN, HIGH, 30000)` waits for Echo to go HIGH and measures how long it stays HIGH — that's the sound wave's round-trip time. The 30000 µs (30 ms) timeout prevents the code from hanging if no echo returns.
3. If `duration == 0`, no echo was received, so the function returns `-1` (no valid reading).
4. Otherwise, converts time to distance using the speed of sound (~0.034 cm/µs), dividing by 2 since the sound travels to the object and back.

### 5. State-Tracking Variables

```arduino
int  lastMotion   = -1;
long lastDistance = -999;
```

Declared globally so they persist across loop iterations. They store the last displayed state, letting the loop detect changes instead of reprinting identical data every cycle. `lastMotion = -1` guarantees the first loop always triggers an update, since real PIR readings are only 0 or 1.

### 6. `loop()` — Runs Continuously

1. **Read PIR sensor**: `digitalRead(PIR_PIN)` — HIGH means motion detected.
2. **Conditionally measure distance**: Only pings the ultrasonic sensor if motion is active, avoiding unnecessary sensor reads.
3. **Decide whether to update the display**:
   - `motionChanged` — true if PIR state flipped since last loop.
   - `shouldUpdate` — true if motion is currently active (keeps distance refreshing live) OR the motion state just changed (so "No motion" prints once, not every cycle).
4. **If motion == HIGH**: Row 0 shows "Motion detected"; row 1 shows "Dist: out range," "WARNING: close!," or "Dist: XX cm" depending on the reading. Same info logged to Serial.
5. **If motion == LOW**: Row 0 shows "No motion," row 1 shows "Waiting..." — printed once per transition, not spammed.
6. **State update**: `lastMotion` and `lastDistance` are updated so next loop's comparisons work correctly.
7. **`delay(300)`**: Throttles the loop to run every 300ms, balancing responsiveness against flicker and excessive Serial output.

### A Subtle but Important Detail

Strings like `"Motion detected "` and `"No motion       "` include trailing spaces. This is intentional — since `lcd.clear()` isn't called every loop (which would cause visible flicker), the padding overwrites any leftover characters from a longer previous message, keeping the display clean without a full screen wipe.

## Customization Ideas

- Adjust `ALERT_DISTANCE` to change the "too close" threshold.
- Adjust the `delay(300)` value to change refresh rate.
- Add a buzzer or LED on another pin to trigger alongside the "WARNING: close!" message.
- Log readings to an SD card or send them over WiFi (e.g., with an ESP32) for remote monitoring.

## License

Feel free to use, modify, and share this project for personal or educational purposes.
