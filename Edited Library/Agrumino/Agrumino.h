/*
  Agrumino.h - Library for Agrumino board - Version 0.2 for Board R3
  Created by giuseppe.broccia@lifely.cc on October 2017. 
  Updated on March 2018

  Future developements:
    - Watering with time duration
    - Add Serial logs in the lib
    - Expose PCA9536 GPIO 2-3-4 Pins
    - Save and read soil water/air values from EEPROM
*/

#ifndef Agrumino_h
#define Agrumino_h

#include "Arduino.h"
#include "EEPROM.h"

class Agrumino {

  public:
    // Constructor
    Agrumino();
    void setup();
    void deepSleepSec(unsigned int sec);
    
    // Public methods GPIO
    void turnWateringOn();
    void turnWateringOff(); // Default Off
    boolean isButtonPressed();
    boolean isAttachedToUSB(); 
    boolean isBatteryCharging();
    boolean isBoardOn();
    void turnBoardOn(); // Also call initBoard()
    void turnBoardOff(); 
    float readBatteryVoltage(); 
    unsigned int readBatteryLevel();
    
    // Public methods I2C
    float readTempC();
    float readTempF();
    void turnLedOn();
    void turnLedOff(); // Default Off
    boolean isLedOn();
    void toggleLed();
    unsigned int readSoil();
    unsigned int readSoilRaw();
    void calibrateSoilWater();
    void calibrateSoilAir();
    void calibrateSoilWater(unsigned int rawValue);
    void calibrateSoilAir(unsigned int rawValue);
    float readLux();

    //methods that allows to read/write from the ESP8266 flash in order to reduce Wifi connection number and to store datas and configurations
    bool initializeMemory();
    bool enableMemory();
    bool getDirty();
    void setDirty(bool isDirty);
    int getFreeMemory();
    int getMaxMemory();
    int getLastAvaiableAddress();
    bool free(int address, int type);
    bool isFree(int address);
    void updateFreeMemory(int byte);
    void updateLastAddress(int byte);
    bool intWrite(int value);
    bool floatWrite(float value);
    bool charWrite(char value);
    bool boolWrite(bool value);
    int intRead(int address);
    float floatRead(int address);
    char charRead(int address);
    bool boolRead(int address);
    bool intArbitraryWrite(int address, int value);
    bool floatArbitraryWrite(int address, float value);
    bool charArbitraryWrite(int address, char value);
    bool boolArbitraryWrite(int address, bool value);
    int getStartAddress();
    void setStartAddress(int val);
    int getHours();
    void incrHours();
    void RSTHours();

 
  private:
    // Private methods
    void setupGpioModes();
    void printLogo();
    void initBoard();
    void initWire();
    void initGpioExpander();
    void initTempSensor();
    void initSoilSensor();
    void initLuxSensor();
    float readBatteryVoltageSingleShot(); 
    boolean checkBattery();



    // Private variables
    unsigned int _soilRawAir;
    unsigned int _soilRawWater;
};

#endif

