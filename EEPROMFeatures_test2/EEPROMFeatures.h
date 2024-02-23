// EEPROMFeatures.h
#ifndef EEPROMFeatures_h
#define EEPROMFeatures_h

#include <Arduino.h>
#include <EEPROM.h>

class EEPROMFeatures
{
  private:
  int _address;
  int _EEPROMsize;
  
  public:
    EEPROMFeatures(int address, int EEPROMsize);
    bool begin();
    void writeString(int address, String str);
    String readString(int address);
};

#endif