
const byte SEGMENTS  = 7;    // Number of segments (A to G)
const byte DIGITS    = 3;    // Number of displays used (three at the moment)
const byte Refresh   = 1;    // Number of millis changes between segments (lower means faster refresh rate)

const byte SEGApin = 53;
const byte SEGBpin = 52;
const byte SEGCpin = 51;
const byte SEGDpin = 50;
const byte SEGEpin = 10;
const byte SEGFpin = 11;
const byte SEGGpin = 12;

// Array to address the segment pins in A-G sequence
byte SEGARRAY[]  = {SEGApin, SEGBpin, SEGCpin, SEGDpin, SEGEpin, SEGFpin, SEGGpin};

// Define pins used by common anodes or common cathodes for each digit
const byte CACC0pin  = A13;   // First digit
const byte CACC1pin  = A14;   // Second digit
const byte CACC2pin  = A15;   // Third digit

// Array allows using any number of digits - now 3 digits
byte  CACCpin[]   = {CACC0pin, CACC1pin, CACC2pin};    // The digit's pin number
byte  DIGIT[DIGITS];                                   // Array to store the current displayed character

// Definitions for common cathode displays (adjusted for COMMON CATHODE)
const byte SEGON     = HIGH;
const byte SEGOFF    = LOW;
const byte CACCON    = LOW;
const byte CACCOFF   = HIGH;

// Segment bit definitions
const byte segA  = bit(0);
const byte segB  = bit(1);
const byte segC  = bit(2);
const byte segD  = bit(3);
const byte segE  = bit(4);
const byte segF  = bit(5);
const byte segG  = bit(6);

// Segment patterns for digits 0-9
const byte char0 = segA + segB + segC + segD + segE + segF;
const byte char1 = segB + segC;
const byte char2 = segA + segB + segD + segE + segG;
const byte char3 = segA + segB + segC + segD + segG;
const byte char4 = segB + segC + segF + segG;
const byte char5 = segA + segC + segD + segF + segG;
const byte char6 = segA + segC + segD + segE + segF + segG;
const byte char7 = segA + segB + segC;
const byte char8 = segA + segB + segC + segD + segE + segF + segG;
const byte char9 = segA + segB + segC + segD + segF + segG;

// Array that maps values 0-9 to their corresponding segment patterns
byte charArray[] = {char0, char1, char2, char3, char4, char5, char6, char7, char8, char9};

// Variables for refresh timing and segment control
unsigned long PREVmillis;
unsigned long CURmillis;
byte SEGCOUNT;  // Segment counter - count up to SEGMENTS value
byte CURSEG;    // Current segment bit position
byte milliCount = 0; // Number of millis changes so far
byte i;         // Loop index for iterating over digits

const int lm35Pin = A0;

void setup() {

  for(i = 0; i < SEGMENTS; ++i) {
    pinMode(SEGARRAY[i], OUTPUT);
    digitalWrite(SEGARRAY[i], SEGOFF);
  }

  for(i = 0; i < DIGITS; ++i) {
    pinMode(CACCpin[i], OUTPUT);
    digitalWrite(CACCpin[i], CACCOFF);
  }

  for(i = 0; i < DIGITS; ++i) {
    DIGIT[i] = char0;
  }

  SEGCOUNT = SEGMENTS - 1;
  CURSEG = bit(SEGMENTS - 1);
  PREVmillis = millis();
}

void loop() {
  int lm35Value = analogRead(lm35Pin);

  float temperature = lm35Value * (5.0 / 1023.0) * 100;
  int tempInt = (int)temperature;

  int hundreds = tempInt / 100;         // Hundreds digit
  int tens = (tempInt / 10) % 10;       // Tens digit
  int ones = tempInt % 10;              // Ones digit

  DIGIT[0] = charArray[hundreds];
  DIGIT[1] = charArray[tens];
  DIGIT[2] = charArray[ones];

  CURmillis = millis();
  if (CURmillis != PREVmillis) {
    milliCount++;
    PREVmillis = CURmillis;
  }
  
  if (milliCount == Refresh) {
    milliCount = 0;

    digitalWrite(SEGARRAY[SEGCOUNT], SEGOFF);

    // Move to the next segment
    CURSEG = CURSEG << 1;
    SEGCOUNT++;
    if (SEGCOUNT == SEGMENTS) {
      SEGCOUNT = 0;
      CURSEG = 1;
    }

    // Update the digit pins based on the current segment pattern
    for(i = 0; i < DIGITS; ++i) {
      if (DIGIT[i] & CURSEG) {
        digitalWrite(CACCpin[i], CACCON);  // Turn on corresponding digit
      } else {
        digitalWrite(CACCpin[i], CACCOFF); // Turn off if segment not active
      }
    }

    // Turn the new segment ON
    digitalWrite(SEGARRAY[SEGCOUNT], SEGON);
  }
}