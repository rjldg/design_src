void setup() {
  noInterrupts();
  CLKPR = 0x80;                           /*Enabling the clock prescaler function*/
  CLKPR = 0x00;                           /*Setting the prescaler to div by 1*/
  interrupts();

  pinMode(13, OUTPUT);
  pinMode(30, OUTPUT);
}
    
void loop() {
  digitalWrite(13, HIGH);
  digitalWrite(30, LOW);
  delay(500);       
  digitalWrite(13, LOW);
  digitalWrite(30, HIGH);
  delay(500); 
}