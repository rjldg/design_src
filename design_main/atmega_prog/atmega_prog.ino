#include <DHT.h>

#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const byte SEGMENTS  = 7;    // Number of segments (A to G)
const byte Refresh   = 1;    // Refresh rate

// TMP36 display (3 digits)
const byte tmpSEGApin = 53;
const byte tmpSEGBpin = 52;
const byte tmpSEGCpin = 51;
const byte tmpSEGDpin = 50;
const byte tmpSEGEpin = 10;
const byte tmpSEGFpin = 11;
const byte tmpSEGGpin = 12;
byte tmpSEGARRAY[]  = {tmpSEGApin, tmpSEGBpin, tmpSEGCpin, tmpSEGDpin, tmpSEGEpin, tmpSEGFpin, tmpSEGGpin};
const byte tmpCACCpin[] = {A13, A14, A15};  // Pins for TMP36 display digits (left to right)
byte tmpDIGIT[3];  // Store the characters to display temperature

// DHT11 display (2 digits)
const byte humSEGApin = 37;
const byte humSEGBpin = 36;
const byte humSEGCpin = 35;
const byte humSEGDpin = 34;
const byte humSEGEpin = 33;
const byte humSEGFpin = 32;
const byte humSEGGpin = 31;
byte humSEGARRAY[]  = {humSEGApin, humSEGBpin, humSEGCpin, humSEGDpin, humSEGEpin, humSEGFpin, humSEGGpin};
const byte humCACCpin[] = {A11, A12};  // Pins for DHT11 display digits (left to right)
byte humDIGIT[2];  // Store the characters to display humidity

// Segment patterns for digits 0-9
const byte segA  = bit(0);
const byte segB  = bit(1);
const byte segC  = bit(2);
const byte segD  = bit(3);
const byte segE  = bit(4);
const byte segF  = bit(5);
const byte segG  = bit(6);
const byte charArray[] = {
  segA + segB + segC + segD + segE + segF, segB + segC, segA + segB + segD + segE + segG,
  segA + segB + segC + segD + segG, segB + segC + segF + segG, segA + segC + segD + segF + segG,
  segA + segC + segD + segE + segF + segG, segA + segB + segC, segA + segB + segC + segD + segE + segF + segG,
  segA + segB + segC + segD + segF + segG
};

// Variables for refresh timing and segment control
unsigned long PREVmillis = 0, CURmillis = 0;
byte SEGCOUNT = SEGMENTS - 1, CURSEG = bit(SEGMENTS - 1);
byte milliCount = 0;

const int tmp36Pin = A1;
const int setThresholdPin = A2;

// BCD pins for temperature threshold
const int tempOnesPins[] = {22, 23, 24, 25};   // Ones digit (BCD)
const int tempTensPins[] = {26, 27, 28, 29};   // Tens digit (BCD)
const int tempSignPin = 13;                    // Sign bit (indicates negative)

// BCD pins for humidity threshold
const int humOnesPins[] = {0, 1, 2, 3};        // Ones digit (BCD)
const int humTensPins[] = {47, 46, 45, 44};    // Tens digit (BCD)

// LED pins
const int blueLED = 6;   // LED to indicate temperature threshold reached (<= threshold)
const int redLED = 7;    // LED to indicate temperature above threshold
const int orangeLED = 8; // LED to indicate humidity above threshold
const int greenLED = 9;  // LED to indicate humidity threshold reached (<= threshold)

void setup() {
  // Initialize DHT11 sensor
  dht.begin();

  pinMode(setThresholdPin, INPUT);

  // TMP36 display setup
  for (byte i = 0; i < SEGMENTS; i++) {
    pinMode(tmpSEGARRAY[i], OUTPUT);
    digitalWrite(tmpSEGARRAY[i], LOW);
  }
  for (byte i = 0; i < 3; i++) {
    pinMode(tmpCACCpin[i], OUTPUT);
    digitalWrite(tmpCACCpin[i], HIGH);  // Common cathode off
  }

  // DHT11 display setup
  for (byte i = 0; i < SEGMENTS; i++) {
    pinMode(humSEGARRAY[i], OUTPUT);
    digitalWrite(humSEGARRAY[i], LOW);
  }
  for (byte i = 0; i < 2; i++) {
    pinMode(humCACCpin[i], OUTPUT);
    digitalWrite(humCACCpin[i], HIGH);  // Common cathode off
  }

  // Setup for temperature and humidity threshold pins (BCD)
  for (int i = 0; i < 4; i++) {
    pinMode(tempOnesPins[i], INPUT);
    pinMode(tempTensPins[i], INPUT);
    pinMode(humOnesPins[i], INPUT);
    pinMode(humTensPins[i], INPUT);
  }
  pinMode(tempSignPin, INPUT);

  // LED setup
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(orangeLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
}

void loop() {
  // Check if the threshold switch is on
  if (digitalRead(setThresholdPin) == HIGH) {
    displaySetThreshold();
  } else {
    displayTemperatureTMP36();
    displayHumidityDHT11();
    controlLEDs();  // Control LEDs based on sensor readings and thresholds
  }

  refreshDisplay();
}

// Display set threshold for temperature and humidity
void displaySetThreshold() {
  // Read temperature threshold (BCD)
  int tempOnes = readBCD(tempOnesPins);
  int tempTens = readBCD(tempTensPins);
  int tempSign = digitalRead(tempSignPin);

  // If the sign bit is set (1), display a negative sign
  if (tempSign == HIGH) {
    tmpDIGIT[0] = segG;  // Negative sign
  } else {
    tmpDIGIT[0] = 0;  // No sign
  }
  tmpDIGIT[1] = charArray[tempTens];
  tmpDIGIT[2] = charArray[tempOnes];

  // Read humidity threshold (BCD)
  int humOnes = readBCD(humOnesPins);
  int humTens = readBCD(humTensPins);

  humDIGIT[0] = charArray[humTens];
  humDIGIT[1] = charArray[humOnes];
}

// Display temperature from TMP36 sensor
void displayTemperatureTMP36() {
  int tmp36Value = analogRead(tmp36Pin);
  float voltage = tmp36Value * (5.025 / 1024.0);
  float temperature = (voltage - 0.5) * 100;  // TMP36 to Celsius

  if (temperature < 0) {
    tmpDIGIT[0] = segG;  // Negative sign
    temperature = -temperature;
  } else {
    tmpDIGIT[0] = (temperature >= 100) ? charArray[(int)temperature / 100] : 0;
  }

  int tempInt = (int)temperature;
  tmpDIGIT[1] = charArray[(tempInt / 10) % 10];
  tmpDIGIT[2] = charArray[tempInt % 10];
}

// Display humidity from DHT11 sensor
void displayHumidityDHT11() {
  int humidity = (int)dht.readHumidity();
  humDIGIT[0] = charArray[(humidity / 10) % 10];
  humDIGIT[1] = charArray[humidity % 10];
}

// Control LEDs based on the sensor readings and thresholds
void controlLEDs() {
  // Read current temperature
  int tmp36Value = analogRead(tmp36Pin);
  float voltage = tmp36Value * (5.025 / 1024.0);
  float temperature = (voltage - 0.5) * 100;  // TMP36 to Celsius

  // Read temperature threshold (BCD)
  int tempOnes = readBCD(tempOnesPins);
  int tempTens = readBCD(tempTensPins);
  int tempThreshold = (tempTens * 10) + tempOnes;
  if (digitalRead(tempSignPin) == HIGH) {
    tempThreshold = -tempThreshold;  // Apply negative sign if required
  }

  // Control blue and red LEDs for temperature
  if (temperature <= tempThreshold) {
    digitalWrite(blueLED, HIGH);  // Blue LED on
    digitalWrite(redLED, LOW);    // Red LED off
  } else {
    digitalWrite(blueLED, LOW);   // Blue LED off
    digitalWrite(redLED, HIGH);   // Red LED on
  }

  // Read current humidity
  int humidity = (int)dht.readHumidity();

  // Read humidity threshold (BCD)
  int humOnes = readBCD(humOnesPins);
  int humTens = readBCD(humTensPins);
  int humThreshold = (humTens * 10) + humOnes;

  // Control green and orange LEDs for humidity
  if (humidity <= humThreshold) {
    digitalWrite(greenLED, HIGH);  // Green LED on
    digitalWrite(orangeLED, LOW);  // Orange LED off
  } else {
    digitalWrite(greenLED, LOW);   // Green LED off
    digitalWrite(orangeLED, HIGH); // Orange LED on
  }
}

// Read BCD value from a set of pins
int readBCD(const int pins[4]) {
  int value = 0;
  for (int i = 0; i < 4; i++) {
    value |= digitalRead(pins[i]) << i;
  }
  return value;
}

// Multiplexing to refresh displays
void refreshDisplay() {
  CURmillis = millis();
  if (CURmillis - PREVmillis >= Refresh) {
    PREVmillis = CURmillis;
    milliCount++;

    // Cycle through the segments
    digitalWrite(tmpSEGARRAY[SEGCOUNT], LOW);
    digitalWrite(humSEGARRAY[SEGCOUNT], LOW);

    CURSEG <<= 1;
    SEGCOUNT++;
    if (SEGCOUNT == SEGMENTS) {
      SEGCOUNT = 0;
      CURSEG = 1;
    }

    // Update TMP36 display
    for (byte i = 0; i < 3; i++) {
      digitalWrite(tmpCACCpin[i], HIGH);
      if (tmpDIGIT[i] & CURSEG) {
        digitalWrite(tmpCACCpin[i], LOW);  // Activate digit
      }
    }

    // Update DHT11 display
    for (byte i = 0; i < 2; i++) {
      digitalWrite(humCACCpin[i], HIGH);
      if (humDIGIT[i] & CURSEG) {
        digitalWrite(humCACCpin[i], LOW);  // Activate digit
      }
    }

    digitalWrite(tmpSEGARRAY[SEGCOUNT], HIGH);
    digitalWrite(humSEGARRAY[SEGCOUNT], HIGH);
  }
}
