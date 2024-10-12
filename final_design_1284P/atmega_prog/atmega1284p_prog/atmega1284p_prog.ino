#include <DHT.h>
#include <ShiftRegister74HC595.h>  // Library for shift register control

// DHT11 setup for humidity readings
#define DHTPIN A1
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Shift register pins for 7-segment display control
ShiftRegister74HC595<1> sr(1, 0, 2);  // DS (Data), SH_CP (Clock), ST_CP (Latch)

// Constants for segment patterns (A to G)
const byte segA = bit(0), segB = bit(1), segC = bit(2), segD = bit(3), segE = bit(4), segF = bit(5), segG = bit(6);
const byte charArray[] = {
  segA + segB + segC + segD + segE + segF,  // 0
  segB + segC,                              // 1
  segA + segB + segD + segE + segG,         // 2
  segA + segB + segC + segD + segG,         // 3
  segB + segC + segF + segG,                // 4
  segA + segC + segD + segF + segG,         // 5
  segA + segC + segD + segE + segF + segG,  // 6
  segA + segB + segC,                       // 7
  segA + segB + segC + segD + segE + segF + segG, // 8
  segA + segB + segC + segD + segF + segG   // 9
};

// Pin definitions for controlling digit select (common cathode)
const byte tmpDigits[] = {3, 4, 5}; // Pins for temperature digits
const byte humDigits[] = {6, 7};     // Pins for humidity digits

// LED pins
const int blueLED = 28, redLED = 29, orangeLED = 30, greenLED = 31;

// BCD pins for temperature and humidity threshold
const int tempOnesPins[] = {8, 9, 10, 11};  // BCD ones for temperature
const int tempTensPins[] = {12, 13, 14, LOW};  // BCD tens for temperature
const int tempSignPin = 15;                   // Sign bit for temperature threshold
const int humOnesPins[] = {16, 17, 18, 19};       // BCD ones for humidity
const int humTensPins[] = {20, 21, 22, 23};   // BCD tens for humidity

unsigned long prevMillis = 0, curMillis = 0;
const byte Refresh = 2;  // Refresh rate for multiplexing

void setup() {
  dht.begin();  // Initialize DHT11 for humidity readings
  
  // Set digit control pins as outputs
  for (byte i = 0; i < 3; i++) pinMode(tmpDigits[i], OUTPUT);  // Temp digit control
  for (byte i = 0; i < 2; i++) pinMode(humDigits[i], OUTPUT);  // Humidity digit control

  // Set LED pins as outputs
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(orangeLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  
  // Set BCD input pins for temperature and humidity thresholds
  for (int i = 0; i < 4; i++) {
    pinMode(tempOnesPins[i], INPUT);
    pinMode(tempTensPins[i], INPUT);
    pinMode(humOnesPins[i], INPUT);
    pinMode(humTensPins[i], INPUT);
  }
  pinMode(tempSignPin, INPUT);
}

void loop() {
  if (digitalRead(A0) == HIGH) {
    displaySetThreshold();
  } else {
    displayTemperature();
    displayHumidity();
    controlLEDs();
  }

  refreshDisplay();
}

// Display the temperature from TMP36 sensor using the 7-segment display
void displayTemperature() {
  float temperature = getTemperature();
  displayDigits(temperature, tmpDigits, 3);
}

// Display the humidity from the DHT11 sensor using the 7-segment display
void displayHumidity() {
  int humidity = (int)dht.readHumidity();
  displayDigits(humidity, humDigits, 2);
}

// Display a floating-point number on a set of 7-segment displays
void displayDigits(float value, const byte* digits, int length) {
  int intValue = (int)value;  // Convert to integer for display
  for (int i = 0; i < length; i++) {
    digitalWrite(digits[i], LOW);
    sr.setAll(charArray[(intValue / (int)pow(10, length - i - 1)) % 10]);
    digitalWrite(digits[i], HIGH);
    delay(5); // Small delay for multiplexing
  }
}

// Control LEDs based on the current temperature and humidity readings against thresholds
void controlLEDs() {
  float temperature = getTemperature();
  
  // Read temperature threshold
  int tempThreshold = (readBCD(tempTensPins) * 10) + readBCD(tempOnesPins);
  if (digitalRead(tempSignPin) == HIGH) {
    tempThreshold = -tempThreshold;  // Apply negative sign
  }

  // Control blue and red LEDs for temperature
  if (temperature <= tempThreshold) {
    digitalWrite(blueLED, HIGH);
    digitalWrite(redLED, LOW);
  } else {
    digitalWrite(blueLED, LOW);
    digitalWrite(redLED, HIGH);
  }

  // Read humidity threshold
  int humidity = (int)dht.readHumidity();
  int humThreshold = (readBCD(humTensPins) * 10) + readBCD(humOnesPins);

  // Control green and orange LEDs for humidity
  if (humidity <= humThreshold) {
    digitalWrite(greenLED, HIGH);
    digitalWrite(orangeLED, LOW);
  } else {
    digitalWrite(greenLED, LOW);
    digitalWrite(orangeLED, HIGH);
  }
}

// Display the temperature and humidity threshold set by the user
void displaySetThreshold() {
  // Display temperature threshold
  int tempOnes = readBCD(tempOnesPins);
  int tempTens = readBCD(tempTensPins);
  int tempSign = digitalRead(tempSignPin);

  if (tempSign == HIGH) {
    sr.setAll(segG);  // Display negative sign
  }
  
  displayDigits(tempTens * 10 + tempOnes, tmpDigits, 3);

  // Display humidity threshold
  int humOnes = readBCD(humOnesPins);
  int humTens = readBCD(humTensPins);

  displayDigits(humTens * 10 + humOnes, humDigits, 2);
}

// Refresh the display for multiplexing 7-segments via shift registers
void refreshDisplay() {
  curMillis = millis();
  if (curMillis - prevMillis >= Refresh) {
    prevMillis = curMillis;
    
    // Cycle through the temperature and humidity segments
    for (byte i = 0; i < 3; i++) digitalWrite(tmpDigits[i], HIGH);  // Turn off all temp digits
    for (byte i = 0; i < 2; i++) digitalWrite(humDigits[i], HIGH);  // Turn off all hum digits
  }
}

// Helper function to read BCD value from pins
int readBCD(const int pins[4]) {
  int value = 0;
  for (int i = 0; i < 4; i++) {
    value |= digitalRead(pins[i]) << i;
  }
  return value;
}

// Retrieve temperature from the TMP36 sensor
float getTemperature() {
  int tmp36Value = analogRead(A0);
  float voltage = tmp36Value * (5.025 / 1024.0);
  return (voltage - 0.5) * 100;  // Convert voltage to temperature in Celsius
}
