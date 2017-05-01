// 4x4 switch matrix scanner
// for ATtiny84
// Using core: https://github.com/SpenceKonde/ATTinyCore
// Pin mapping: Clockwise
// PaulRB Jan 2017

const byte rowPins[4] = {5, 4, 3, 2};
const byte colPins[4] = {10, 8, 9, 7};

unsigned int switchData; // Holds all 16 switch states in binary, 0 = switch pressed

void setup() {
  Serial.begin(9600); // Outputs on pin 1
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
  if (newSwitchData != switchData) {
    switchData = newSwitchData;
    // Send new switch state data as two bytes
    Serial.print((char) lowByte(switchData));
    Serial.print((char) highByte(switchData));
  }
}
