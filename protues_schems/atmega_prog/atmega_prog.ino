#include <DHT.h>

#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const byte SEGMENTS  = 7;    // Number of segments (A to G)
const byte Refresh   = 1;    // Refresh rate (the lower the faster)

// TMP36 display (3 digits or 7segments)
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

// DHT11 display (2 digits or 7segments)
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
const byte charArray[] = {segA + segB + segC + segD + segE + segF, segB + segC, segA + segB + segD + segE + segG,
                          segA + segB + segC + segD + segG, segB + segC + segF + segG, segA + segC + segD + segF + segG,
                          segA + segC + segD + segE + segF + segG, segA + segB + segC, segA + segB + segC + segD + segE + segF + segG,
                          segA + segB + segC + segD + segF + segG};

unsigned long PREVmillis = 0, CURmillis = 0;
byte SEGCOUNT = SEGMENTS - 1, CURSEG = bit(SEGMENTS - 1);
byte milliCount = 0;

const int tmp36Pin = A1;

const int setTempPin = 2;

int tempThreshold = 0;  // Default threshold value set for temperature

void setup() {
  // Initialize DHT11 sensor
  dht.begin();

  pinMode(setTempPin, INPUT);

  // for TMP36 display segments
  for (byte i = 0; i < SEGMENTS; i++) {
    pinMode(tmpSEGARRAY[i], OUTPUT);
    digitalWrite(tmpSEGARRAY[i], LOW);
  }
  for (byte i = 0; i < 3; i++) {
    pinMode(tmpCACCpin[i], OUTPUT);
    digitalWrite(tmpCACCpin[i], HIGH);  // Common cathode off
  }

  // for DHT11 display segments
  for (byte i = 0; i < SEGMENTS; i++) {
    pinMode(humSEGARRAY[i], OUTPUT);
    digitalWrite(humSEGARRAY[i], LOW);
  }
  for (byte i = 0; i < 2; i++) {
    pinMode(humCACCpin[i], OUTPUT);
    digitalWrite(humCACCpin[i], HIGH);  // Common cathode off
  }
}

void loop() {
  displayTemperatureTMP36();
  
  displayHumidityDHT11();
  
  refreshDisplay();
}

// Display temperature from TMP36 sensor
void displayTemperatureTMP36() {
  int setTempThreshold = digitalRead(setTempPin);

  if (setTempThreshold == LOW) {
    int tmp36Value = analogRead(tmp36Pin);
    float voltage = tmp36Value * (5.025 / 1024.0);  // TMP36 analog output to standardized voltage
    float temperature = (voltage - 0.5) * 100;     // TMP36 to degrees celsius

    if (temperature < 0) {
      tmpDIGIT[0] = segG;  // Negative sign
      temperature = -(temperature - 1);  // Convert to positive for display
    } else {
      tmpDIGIT[0] = (temperature >= 100) ? charArray[(int)temperature / 100] : 0;  // Handle 100+
    }

    int tempInt = (int)temperature;
    tmpDIGIT[1] = charArray[(tempInt / 10) % 10];  // Tens
    tmpDIGIT[2] = charArray[tempInt % 10];         // Ones
  } else {
    if (tempThreshold < 0) {
      tmpDIGIT[0] = segG;  // Negative sign
      tempThreshold = -(tempThreshold - 1);  // Convert to positive for display
    } else {
      tmpDIGIT[0] = (tempThreshold >= 100) ? charArray[(int)tempThreshold / 100] : 0;  // Handle 100+
    }

    tmpDIGIT[1] = charArray[(tempThreshold / 10) % 10];
    tmpDIGIT[2] = charArray[tempThreshold % 10];
  }
  
}

// Display humidity from DHT11 sensor
void displayHumidityDHT11() {
  int humidity = (int)dht.readHumidity();

  humDIGIT[0] = charArray[(humidity / 10) % 10];  // tens
  humDIGIT[1] = charArray[humidity % 10];         // ones
}

// Multiplexing 7-segments to refresh displays
void refreshDisplay() {
  CURmillis = millis();
  if (CURmillis - PREVmillis >= Refresh) {
    PREVmillis = CURmillis;
    milliCount++;

    // cycle through the segments
    digitalWrite(tmpSEGARRAY[SEGCOUNT], LOW);
    digitalWrite(humSEGARRAY[SEGCOUNT], LOW);

    CURSEG <<= 1;
    SEGCOUNT++;
    if (SEGCOUNT == SEGMENTS) {
      SEGCOUNT = 0;
      CURSEG = 1;
    }

    // update TMP36 display readings
    for (byte i = 0; i < 3; i++) {
      digitalWrite(tmpCACCpin[i], HIGH);
      if (tmpDIGIT[i] & CURSEG) {
        digitalWrite(tmpCACCpin[i], LOW);  // Activate digit for common cathode
      }
    }

    // Update DHT11 display
    for (byte i = 0; i < 2; i++) {
      digitalWrite(humCACCpin[i], HIGH);
      if (humDIGIT[i] & CURSEG) {
        digitalWrite(humCACCpin[i], LOW);  // Activate digit for common cathode
      }
    }

    digitalWrite(tmpSEGARRAY[SEGCOUNT], HIGH);  // Turn on a segment for temperature display
    digitalWrite(humSEGARRAY[SEGCOUNT], HIGH);  // Turn on a segment for humidity display
  }
}
