#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

#define ONE_WIRE_BUS A0
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

#define DHTPIN A1  // DHT11 data pin connected to A1
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Pins for controlling the 74HC595 shift register
const int latchPin = 2;   // Pin connected to ST_CP of 74HC595
const int clockPin = 0;   // Pin connected to SH_CP of 74HC595
const int dataPin = 1;    // Pin connected to DS of 74HC595

// Pins for digit selection (common cathodes for 5 digits)
const int digitPins[5] = {3, 4, 5, 6, 7};  // Pins to control the 5 common cathodes of the displays

// Seven-segment digit patterns for common cathode displays
const byte digit[12] = {
  B11000000, // 0
  B11111001, // 1
  B10100100, // 2
  B10110000, // 3
  B10011001, // 4
  B10010010, // 5
  B10000010, // 6
  B11111000, // 7
  B10000000, // 8
  B10010000, // 9
  B11111111, // Blank
  B10111111  // '-' (minus sign)
};

// Variables for handling the display
int digitBuffer[5] = {10, 0, 0, 0, 0};  // Buffer to store values to display (default blank)
int digitScan = 0;  // Keeps track of which digit is being displayed
int soft_scaler = 0;  // Scaler for controlling the refresh rate
float tempC;  // Temperature in Celsius
int tmp;  // Temperature value to display
float humidity;  // Humidity percentage
int hum;  // Humidity value to display
boolean sign = false;  // Indicates if the temperature is negative

// LED and threshold pin configurations
const int setThresholdPin = A2;
const int blueLED = 6;
const int redLED = 7;
const int orangeLED = 8;
const int greenLED = 9;

// BCD threshold pins
const int tempOnesPins[] = {8, 9, 10, 11};
const int tempTensPins[] = {12, 13, 14, LOW};
const int tempSignPin = 15;
const int humOnesPins[] = {16, 17, 18, 19};
const int humTensPins[] = {20, 21, 22, 23};

void setup() {
  // Initialize temperature sensor and DHT11
  sensors.begin();
  sensors.getAddress(insideThermometer, 0);
  dht.begin();  // Initialize DHT11 sensor

  // Setup shift register and digit control pins
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  for (int i = 0; i < 5; i++) {
    pinMode(digitPins[i], OUTPUT);
    digitalWrite(digitPins[i], HIGH);  // Start with all digits off (common cathode)
  }

  // Set up Timer2 for refreshing the display
  TCCR2A = 0;
  TCCR2B = (1 << CS21);  // Prescaler of 8
  TIMSK2 = (1 << TOIE2);  // Enable overflow interrupt
  TCNT2 = 0;

  pinMode(setThresholdPin, INPUT);

  // LED setup
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(orangeLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
}

ISR(TIMER2_OVF_vect) {
  soft_scaler++;
  if (soft_scaler == 15) {
    refreshDisplay();
    soft_scaler = 0;
  }
}

void refreshDisplay() {
  // Turn off all digits (set common cathodes HIGH)
  for (byte i = 0; i < 5; i++) {
    digitalWrite(digitPins[i], HIGH);
  }

  // Prepare to send segment data to the shift register
  digitalWrite(latchPin, LOW);  // Latch low to start shifting data
  shiftOut(dataPin, clockPin, MSBFIRST, ~digit[digitBuffer[digitScan]]);  // Shift out segment data (active high for common cathode)
  digitalWrite(latchPin, HIGH);  // Latch high to store data in the shift register

  // Enable the current digit (set common cathode LOW)
  digitalWrite(digitPins[digitScan], LOW);

  // Move to the next digit for the next refresh cycle
  digitScan++;
  if (digitScan >= 5) {
    digitScan = 0;
  }
}

void loop() {
  // Check if the threshold switch is on
  if (digitalRead(setThresholdPin) == HIGH) {
    displaySetThreshold();
  } else {
    displayTemperature();
    displayHumidity();
    controlLEDOutputs();  // Control LEDs based on sensor readings and thresholds
  }

  // Let the timer handle multiplexing, no need for a delay here
}

// Display temperature from DS18B20 sensor
void displayTemperature() {
  sensors.requestTemperatures();
  tempC = sensors.getTempC(insideThermometer);
  tmp = int(tempC);  // Multiply by 10 to display one decimal point

  if (tempC < 0) {
    sign = true;
    tmp = abs(tmp);  // Take absolute value for display
  } else {
    sign = false;
  }

  // Fill digit buffer for temperature display
  digitBuffer[2] = tmp % 10;  // Ones place
  digitBuffer[1] = (tmp / 10) % 10;  // Tens place
  digitBuffer[0] = sign ? 11 : 10;  // Display '-' for negative, blank for positive
}

// Display humidity from DHT11 sensor
void displayHumidity() {
  humidity = dht.readHumidity();
  hum = int(humidity);

  // Fill digit buffer for humidity display
  digitBuffer[4] = hum % 10;  // Ones place
  digitBuffer[3] = (hum / 10) % 10;  // Tens place
}

// Control LEDs based on temperature and humidity thresholds
void controlLEDOutputs() {
  // Read temperature threshold
  int tempOnes = readBCD(tempOnesPins);
  int tempTens = readBCD(tempTensPins);
  int tempThreshold = (tempTens * 10) + tempOnes;
  if (digitalRead(tempSignPin) == HIGH) {
    tempThreshold = -tempThreshold;
  }

  // Control blue and red LEDs for temperature
  if (tempC <= tempThreshold) {
    digitalWrite(blueLED, HIGH);
    digitalWrite(redLED, LOW);
  } else {
    digitalWrite(blueLED, LOW);
    digitalWrite(redLED, HIGH);
  }

  // Read humidity threshold
  int humOnes = readBCD(humOnesPins);
  int humTens = readBCD(humTensPins);
  int humThreshold = (humTens * 10) + humOnes;

  // Control green and orange LEDs for humidity
  if (humidity <= humThreshold) {
    digitalWrite(greenLED, HIGH); 
    digitalWrite(orangeLED, LOW);
  } else {
    digitalWrite(greenLED, LOW);
    digitalWrite(orangeLED, HIGH);
  }
}

// Display set threshold for temperature and humidity
void displaySetThreshold() {
  // Read temperature threshold
  int tempOnes = readBCD(tempOnesPins);
  int tempTens = readBCD(tempTensPins);
  int tempSign = digitalRead(tempSignPin);

  // Display temperature threshold
  digitBuffer[0] = (tempSign == HIGH) ? 11 : 10;  // '-' for negative sign
  digitBuffer[1] = tempTens;
  digitBuffer[2] = tempOnes;

  // Read humidity threshold
  int humOnes = readBCD(humOnesPins);
  int humTens = readBCD(humTensPins);

  // Display humidity threshold
  digitBuffer[3] = humTens;
  digitBuffer[4] = humOnes;
}

// Read BCD value from a set of pins
int readBCD(const int pins[4]) {
  int value = 0;
  for (int i = 0; i < 4; i++) {
    value |= digitalRead(pins[i]) << i;
  }
  return value;
}
