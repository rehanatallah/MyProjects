//EEPROMFeatures.cpp
#include "EEPROMFeatures.h"

EEPROMFeatures::EEPROMFeatures(int address, int EEPROMsize){
  _address = address;
  _EEPROMsize = EEPROMsize;
}

bool EEPROMFeatures::begin(){
  if (!EEPROM.begin(_EEPROMsize)) {
    Serial.println("Failed to initialise EEPROM, Restarting.....");
    delay(1000);
    return false;
  }
  return true;
}

void EEPROMFeatures::writeString(int address, String str){
  if (str.length() >= _EEPROMsize - address - 1){
    Serial.println("String too long");
    return;
  }
  EEPROM.write(address + _address, str.length());
  for (int i = 0; i < str.length(); ++i) {
    EEPROM.write(address + _address + 1 + i, str[i]);
  }
  EEPROM.commit(); // Simpan perubahan ke EEPROM
}
String EEPROMFeatures::readString(int address){
  int strLength = EEPROM.read(address + _address);
  String str;
  for (int i = 0; i < strLength; ++i) {
    str += char(EEPROM.read(address + _address + 1 + i));
  }
  return str;
}