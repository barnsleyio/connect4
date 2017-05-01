// 4x4 switch matrix scanner
// for ATtiny84 @ 8MHz
// Using core: https://github.com/SpenceKonde/ATTinyCore
// Pin mapping: Clockwise (non-"alternative")
// PaulRB Jan 2017

const byte rowPins[4] = {5, 4, 3, 0};
const byte colPins[4] = {10, 9, 8, 7};

unsigned int dataBuffer[8]; // Holds all 16 switch states in binary, 0 = switch pressed
byte bufferStart = 0;
byte bufferEnd = 7;


void setup() {
  Serial.begin(57600); // TX on pin 2 (AIN1), RX on pin 1 (AIN0)
  for (byte i = 0; i < 4; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
    pinMode(colPins[i], INPUT_PULLUP);
  }
}

void loop() {

  unsigned int newSwitchData = 0;

  // Scan each column in turn
  for (byte j = 0; j < 4; j++) {
    
    // Enable switches in this column to pull a row LOW
    pinMode(colPins[j], OUTPUT);
    digitalWrite(colPins[j], LOW);

    // Read the switches on this row
    for (byte i = 0; i < 4; i++) {
      newSwitchData = newSwitchData << 1 | digitalRead(rowPins[i]);
    }

    // Disable the column again
    pinMode(colPins[j], INPUT_PULLUP);
  }
  
  // Has the switch state data changed?
  if (newSwitchData != dataBuffer[bufferEnd]) {
    //Save it in the buffer
    if (++bufferEnd > 7) bufferEnd = 0;
    dataBuffer[bufferEnd] = newSwitchData;
  }
    
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '?') {
      delay(1);
      // Send switch state data from the buffer as two bytes
      Serial.print((char) lowByte(dataBuffer[bufferStart]));
      Serial.print((char) highByte(dataBuffer[bufferStart]));
      // Unless this is the only data in the buffer, move to the next data in the buffer
      if (bufferStart != bufferEnd) {
        if (++bufferStart > 7) bufferStart = 0;
      }
    }
  }
}

