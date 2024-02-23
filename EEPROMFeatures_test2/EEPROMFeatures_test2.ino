#include "EEPROMFeatures.h"

EEPROMFeatures EEPROMF(0,512);

void setup() {
  Serial.begin(115200);
  if(EEPROMF.begin() == false){
    ESP.restart();
  }
  EEPROMF.writeString(0, "Hello");
  Serial.println(EEPROMF.readString(0));
}

void loop() {
  // put your main code here, to run repeatedly:

}
