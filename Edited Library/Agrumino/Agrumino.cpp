/*
  Agrumino.cpp - Library for Agrumino board - Version 0.2 for Board R3
  Created by giuseppe.broccia@lifely.cc on October 2017.
  Updated on March 2018

  For details @see Agrumino.h
*/

#include "Agrumino.h"
#include <Wire.h>
#include "libraries/MCP9800/MCP9800.cpp"
#include "libraries/PCA9536_FIX/PCA9536_FIX.cpp" // PCA9536.h lib has been modified (REG_CONFIG renamed to REG_CONFIG_PCA) to avoid name clashing with mcp9800.h
#include "libraries/MCP3221/MCP3221.cpp"


// PINOUT Agrumino        Implemented
#define PIN_SDA          2 // [X] BOOT: Must be HIGH at boot
#define PIN_SCL         14 // [X] 
#define PIN_PUMP        12 // [X] 
#define PIN_BTN_S1       4 // [X] Same as Internal WT8266 LED
#define PIN_USB_DETECT   5 // [X] 
#define PIN_MOSFET      15 // [X] BOOT: Must be LOW at boot
#define PIN_BATT_STAT   13 // [X] 
#define PIN_LEVEL        0 // [ ] BOOT: HIGH for Running and LOW for Program

// Addresses I2C sensors       Implemented
#define I2C_ADDR_SOIL      0x4D // [X] TODO: Save air and water values to EEPROM
#define I2C_ADDR_LUX       0x44 // [X] 
#define I2C_ADDR_TEMP      0x48 // [X] 
#define I2C_ADDR_GPIO_EXP  0x41 // [-] BOTTOM LED OK, TODO: GPIO 2-3-4 

//Addresses for flash utility
#define DIRTY 0 //tells the user if the memory is "dirty" A.K.A if there's some data that need to be pushed. Must be manually handled by the user
#define LASTFREEADD 1 //address that tells the user the last free index (reliable only if writing sequentially)
#define FREE_MEMORY 5 //address containing how many Bites of memory are free
#define START_ADDRESS 10 //starting address to read the datas (for RST survive)
#define HOURS 14 //register for keeping the amount of hours since last data push
#define USERSPACE 20 //the index from which the user can start writing data
#define MAX_MEMORY 4096 //fixed max flash size

////////////
// CONFIG //
////////////

// GPIO Expander
#define IO_PCA9536_LED                IO0 // Bottom Green led is connected to GPIO0 of the PCA9536 GPIO expander
// Soil values from a brand new device with the sensor in the air / immersed by 3/4 in water
#define DEFAULT_SOIL_RAW_AIR         3400 // Lifely R3 Capacitive Flat
#define DEFAULT_SOIL_RAW_WATER       1900 // Lifely R3 Capacitive Flat
// #define DEFAULT_SOIL_RAW_AIR         3400 // Lifely R3 Capacitive with holes
// #define DEFAULT_SOIL_RAW_WATER       1650 // Lifely R3 Capacitive with holes
// Battery
#define BATTERY_MILLIVOLT_LEVEL_0    3500 // Voltage of a fully discharged battery (Safe value, cut-off of a LIR2450 is 2.75 V)
#define BATTERY_MILLIVOLT_LEVEL_100  4200 // Voltage of a fully charged battery
#define BATTERY_VOLT_DIVIDER_Z1      1800 // Value of the Z1(R25) resistor in the Voltage divider used for read the batt voltage.
#define BATTERY_VOLT_DIVIDER_Z2       424 // 470 (Original) // Value of the Z2(R26) resistor. Adjusted considering the ADC internal resistance.
#define BATTERY_VOLT_SAMPLES           20 // Number of reading needed to calculate the battery voltage

///////////////
// Variables //
///////////////

MCP9800 mcpTempSensor;
PCA9536 pcaGpioExpander;
MCP3221 mcpSoilSensor(I2C_ADDR_SOIL);
unsigned int _soilRawAir;
unsigned int _soilRawWater;

/////////////////
// Constructor //
/////////////////

Agrumino::Agrumino() {
}

void Agrumino::setup() {
  setupGpioModes();
  printLogo();
  // turnBoardOn(); // Decomment to have the board On by Default
}

void Agrumino::deepSleepSec(unsigned int sec) {
  if (sec > 4294) {
    // ESP.deepSleep argument is an unsigned int, so the max allowed walue is 0xffffffff (4.294.967.295).
    sec = 4294;
    Serial.println("Warning: deepSleep can be max 4294 sec (~71 min). Value has been constrained!");
  }

  Serial.print("\nGoing to deepSleep for ");
  Serial.print(sec);
  Serial.println(" seconds... (ー。ー) zzz\n");
  ESP.deepSleep(sec * 1000000); // microseconds
}

/////////////////////////
// Public methods GPIO //
/////////////////////////

boolean Agrumino::isAttachedToUSB() {
  return digitalRead(PIN_USB_DETECT) == HIGH;
}

boolean Agrumino::isBatteryCharging() {
  return digitalRead(PIN_BATT_STAT) == LOW;
}

boolean Agrumino::isButtonPressed() {
  return digitalRead(PIN_BTN_S1) == LOW;
}

boolean Agrumino::isBoardOn() {
  return digitalRead(PIN_MOSFET) == HIGH;
}

void Agrumino::turnBoardOn() {
  if (!isBoardOn()) {
    digitalWrite(PIN_MOSFET, HIGH);
    delay(5); // Ensure that the ICs are booted up properly
    initBoard();
    checkBattery();
  }
}

void Agrumino::turnBoardOff() {
  digitalWrite(PIN_MOSFET, LOW);
}

void Agrumino::turnWateringOn() {
  digitalWrite(PIN_PUMP, HIGH);
}

void Agrumino::turnWateringOff() {
  digitalWrite(PIN_PUMP, LOW);
}

////////////////////////
// Public methods I2C //
////////////////////////

float Agrumino::readTempC() {
  return mcpTempSensor.readCelsiusf();
}

float Agrumino::readTempF() {
  return mcpTempSensor.readFahrenheitf();
}

void Agrumino::turnLedOn() {
  pcaGpioExpander.setState(IO_PCA9536_LED, IO_HIGH);
}

void Agrumino::turnLedOff() {
  pcaGpioExpander.setState(IO_PCA9536_LED, IO_LOW);
}

boolean Agrumino::isLedOn() {
  return pcaGpioExpander.getState(IO_PCA9536_LED) == 1;
}

void Agrumino::toggleLed() {
  pcaGpioExpander.toggleState(IO_PCA9536_LED);
}

void Agrumino::calibrateSoilWater() {
  _soilRawWater = readSoilRaw();
}

void Agrumino::calibrateSoilAir() {
  _soilRawAir = readSoilRaw();
}

void Agrumino::calibrateSoilWater(unsigned int rawValue) {
  _soilRawWater = rawValue;
}

void Agrumino::calibrateSoilAir(unsigned int rawValue) {
  _soilRawAir = rawValue;
}

unsigned int Agrumino::readSoil() {
  unsigned int soilRaw = readSoilRaw();
  soilRaw = constrain(soilRaw, _soilRawWater, _soilRawAir);
  return map(soilRaw, _soilRawAir, _soilRawWater, 0, 100);
}

float Agrumino::readLux() {
  // Logic for Light-to-Digital Output Sensor ISL29003
  Wire.beginTransmission(I2C_ADDR_LUX);
  Wire.write(0x02); // Data registers are 0x02->LSB and 0x03->MSB
  Wire.endTransmission();
  Wire.requestFrom(I2C_ADDR_LUX, 2); // Request 2 bytes of data
  unsigned int data;
  if (Wire.available() == 2) {
    byte lsb = Wire.read();
    byte  msb = Wire.read();
    data = (msb << 8) | lsb;
  } else {
    Serial.println("readLux Error!");
    return 0;
  }
  // Convert the data from the ADC to lux
  // 0-64000 is the selected range of the ALS (Lux)
  // 0-65536 is the selected range of the ADC (16 bit)
  return (64000.0 * (float) data) / 65536.0;
}

float Agrumino::readBatteryVoltage() {
  float voltSum = 0.0;
  for (int i = 0; i < BATTERY_VOLT_SAMPLES; ++i) {
    voltSum += readBatteryVoltageSingleShot();
  }
  float volt = voltSum / BATTERY_VOLT_SAMPLES;
  // Serial.println("readBatteryVoltage: " + String(volt) + " V");
  return volt;
}

unsigned int Agrumino::readBatteryLevel() {
  float voltage = readBatteryVoltage();
  unsigned int milliVolt = (int) (voltage * 1000.0);
  milliVolt = constrain(milliVolt, BATTERY_MILLIVOLT_LEVEL_0, BATTERY_MILLIVOLT_LEVEL_100);
  return map(milliVolt, BATTERY_MILLIVOLT_LEVEL_0, BATTERY_MILLIVOLT_LEVEL_100, 0, 100);
}

/////////////////////
// Private methods //
/////////////////////

void Agrumino::initGpioExpander() {
  Serial.print("initGpioExpander → ");
  byte result = pcaGpioExpander.ping();;
  if (result == 0) {
    pcaGpioExpander.reset();
    pcaGpioExpander.setMode(IO_PCA9536_LED, IO_OUTPUT); // Back green LED
    pcaGpioExpander.setState(IO_PCA9536_LED, IO_LOW);
    Serial.println("OK");
  } else {
    Serial.println("FAIL!");
  }
}

void Agrumino::initTempSensor() {
  Serial.print("initTempSensor   → ");
  boolean success = mcpTempSensor.init(true);
  if (success) {
    mcpTempSensor.setResolution(MCP_ADC_RES_11); // 11bit (0.125c)
    mcpTempSensor.setOneShot(true);
    Serial.println("OK");
  } else {
    Serial.println("FAIL!");
  }
}

void Agrumino::initSoilSensor() {
  Serial.print("initSoilSensor   → ");
  byte response = mcpSoilSensor.ping();;
  if (response == 0) {
    mcpSoilSensor.reset();
    mcpSoilSensor.setSmoothing(EMAVG);
    // mcpSoilSensor.setVref(3300); This will make the reading of the MCP3221 voltage accurate. Currently is not needed because we need just a range
    _soilRawAir = DEFAULT_SOIL_RAW_AIR;
    _soilRawWater = DEFAULT_SOIL_RAW_WATER;
    Serial.println("OK");
  } else {
    Serial.println("FAIL!");
  }
}

void Agrumino::initLuxSensor() {
  // Logic for Light-to-Digital Output Sensor ISL29003
  Serial.print("initLuxSensor    → ");
  Wire.beginTransmission(I2C_ADDR_LUX);
  byte result = Wire.endTransmission();
  if (result == 0) {
    Wire.beginTransmission(I2C_ADDR_LUX);
    Wire.write(0x00); // Select "Command-I" register
    Wire.write(0xA0); // Select "ALS continuously" mode
    Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_LUX);
    Wire.write(0x01); // Select "Command-II" register
    Wire.write(0x03); // Set range = 64000 lux, ADC 16 bit
    Wire.endTransmission();
    Serial.println("OK");
  } else {
    Serial.println("FAIL!");
  }
}

unsigned int Agrumino::readSoilRaw() {
  return mcpSoilSensor.getVoltage();
}

float Agrumino::readBatteryVoltageSingleShot() {
  float z1 = (float) BATTERY_VOLT_DIVIDER_Z1;
  float z2 = (float) BATTERY_VOLT_DIVIDER_Z2;
  float vread = (float) analogRead(A0) / 1024.0; // RAW Value from the ADC, Range 0-1V
  float volt = ((z1 + z2) / z2) * vread;
  return volt;
}

// Return true if the battery is ok
// Return false and put the ESP to sleep if not
boolean Agrumino::checkBattery() {
  if (readBatteryLevel() > 0) {
    return true;
  } else {
    Serial.print("\nturnBoardOn Fail! Battery is too low!!!\n");
    deepSleepSec(3600); // Sleep for 1 hour
    return false;
  }
}

/*initializes the Agrumino memory by putting (255) all over it's flash, then
 * sets the reserved addresses. Returns false if the
   board isn't active*/
bool Agrumino::initializeMemory()
{
    if(isBoardOn())
    {
        EEPROM.begin(MAX_MEMORY);

        //writing 255 on all the address, excluding the reserved ones
        for(int i=0; i<MAX_MEMORY; i++)
        {
            EEPROM.write(i,255);
            EEPROM.commit();
        }
        EEPROM.put(LASTFREEADD,USERSPACE); //setting the first address in which the user can write
        EEPROM.commit();
        int m = MAX_MEMORY-20;
        EEPROM.put(FREE_MEMORY,m); //setting the free memory
        EEPROM.commit();
        EEPROM.put(START_ADDRESS,USERSPACE); //setting the start address as the first user address, since the memory is empty
        EEPROM.commit();
        RSTHours(); //setting the hours without push as 0
        setDirty(false); //flagging the memory as "clean"
        return true;
    }
    else
        return false;
}

//useful to use the memory without re-initializing it (i.e.: after a RST)
bool Agrumino::enableMemory()
{
    EEPROM.begin(MAX_MEMORY);
}

//returns a boolean depending on the presence on datas on the flash
bool Agrumino::getDirty()
{
    return (bool) EEPROM.read(DIRTY);
}

/*Sets the dirty address of the flash storage to either 1 or 0.
  This tells the user if there are some datas that still needs to be used (i.e pushed on the server)
  or not.*/
void Agrumino::setDirty(bool isDirty)
{
    EEPROM.put(DIRTY,isDirty);
    EEPROM.commit();
}

//returns the free memory amount
int Agrumino::getFreeMemory()
{
    int result = 0;
    EEPROM.get(FREE_MEMORY,result);
    return (int) result;
}

//returns the maximum memory of the board (reserved registers included) in Bytes
int Agrumino::getMaxMemory()
{
    return MAX_MEMORY;
}

//returns the last address that is free. This should be used in case of sequential write only
int Agrumino::getLastAvaiableAddress()
{
    int result = 0;
    EEPROM.get(LASTFREEADD,result);
    return (int) result;
}

/*frees up an address, by putting 255 in it. Note that this invalidates the
 * information in the LASTFREEADD address (that assumes sequentiality in writes)
  Other than the address that must be cleaned it takes also the type of data:
  0 = 1B data (int/char/bool), 1 = 4B data (float)

  returns a boolean depending on if the operation was successful*/
bool Agrumino::free(int address, int type)
{
    //freeing the 1B datas
    if(type==0)
    {
        EEPROM.write(address,255);
        EEPROM.commit();
        EEPROM.put(FREE_MEMORY,(getFreeMemory()+1));
        EEPROM.commit();
        return true;
    }
    //float case
    if(type==1)
    {
        EEPROM.write(address,255);
        EEPROM.write(address+1,255);
        EEPROM.write(address+2,255);
        EEPROM.write(address+3,255);
        EEPROM.commit();
        EEPROM.put(FREE_MEMORY,(getFreeMemory()+4));
        EEPROM.commit();
        return true;
    }

    return false;

}

//returns a boolean depeinding on if the passed address contains datas or not
bool Agrumino::isFree(int address)
{
    //preventing the user from accessing reserved addresses
    if(address<10)
        return false;
    if(EEPROM.read(address)==255)
        return true;
    return false;

}

/*tells the user the first address "of interest".
  This information is initialized the first time as the first user address, but must
  be managed by the user itself. Its purpose is to tell the user where the data that
  he wants to read begins.*/
int Agrumino::getStartAddress()
{
    return EEPROM.read(START_ADDRESS);
}

//setter for the previous address: must be handled by the user
void Agrumino::setStartAddress(int val)
{
    EEPROM.put(START_ADDRESS,val);
    EEPROM.commit();
}

//returns to the user how many hours has been passed since the last data push
int Agrumino::getHours()
{
    return EEPROM.read(HOURS);
}

//increaser for the hours register
void Agrumino::incrHours()
{
    int h = getHours();
    EEPROM.put(HOURS,h+1);
    EEPROM.commit();
}

//resets the hours register
void Agrumino::RSTHours()
{
    EEPROM.put(HOURS,0);
    EEPROM.commit();
}

/*the following functions handles the sequential writing and reading of datas.
  This means that the flash memory is threated as a linear list, and it is not
  possible to manually write specific bytes. If the user wants to write to specific
  bytes the arbitrary functions should be used (scroll down). It must be noted that
  the use of the sequential write functions give sense to the LASTFREEADD byte, that
  shoudln't be used otherwise*/

bool Agrumino::intWrite(int value)
{
    //getting where to write
    int lastAvaiableAddress = getLastAvaiableAddress();
    int freeMemory = getFreeMemory(); //and free memory
    if(freeMemory<1) //checking if there's enough memory avaiable
        return false;

    //writing the data, updating the last free address and free memory
    EEPROM.write(lastAvaiableAddress,value);
    EEPROM.put(FREE_MEMORY,(getFreeMemory()-1));
    EEPROM.commit();
    EEPROM.put(LASTFREEADD,(getLastAvaiableAddress()+1));
    EEPROM.commit();

    //setting the dirty flag
    setDirty(true);
    return true;
}

bool Agrumino::floatWrite(float value)
{
    int lastAvaiableAddress = getLastAvaiableAddress();
    int freeMemory = getFreeMemory();
    if(freeMemory<4)
        return false;


    //splitting a float in it's 4 byte's array
    union u_tag
    {
        byte b[4];
        float fval;
    } u;

    //this initializes the array too
    u.fval=value;



    //writing the 4 bytes manually and updating the flags and addresses
    EEPROM.write(lastAvaiableAddress  , u.b[0]);
    EEPROM.write(lastAvaiableAddress+1, u.b[1]);
    EEPROM.write(lastAvaiableAddress+2, u.b[2]);
    EEPROM.write(lastAvaiableAddress+3, u.b[3]);

    EEPROM.put(FREE_MEMORY,(getFreeMemory()-4));
    EEPROM.commit();
    EEPROM.put(LASTFREEADD,(getLastAvaiableAddress()+4));
    EEPROM.commit();

    setDirty(true);
    return true;
}

bool Agrumino::charWrite(char value)
{
    int lastAvaiableAddress = getLastAvaiableAddress();
    int freeMemory = getFreeMemory();
    if(freeMemory<1)
        return false;

    EEPROM.write(lastAvaiableAddress,value);
    EEPROM.commit();

    EEPROM.put(FREE_MEMORY,(getFreeMemory()-1));
    EEPROM.commit();
    EEPROM.put(LASTFREEADD,(getLastAvaiableAddress()+1));
    EEPROM.commit();

    setDirty(true);
    return true;
}

bool Agrumino::boolWrite(bool value)
{
    int lastAvaiableAddress = getLastAvaiableAddress();
    int freeMemory = getFreeMemory();
    if(freeMemory<1)
        return false;

    EEPROM.write(lastAvaiableAddress,value);
    EEPROM.commit();

    EEPROM.put(FREE_MEMORY,(getFreeMemory()-1));
    EEPROM.commit();
    EEPROM.put(LASTFREEADD,(getLastAvaiableAddress()+1));
    EEPROM.commit();

    setDirty(true);
    return true;
}

/*the following functions allow the user to read stored data and returns -1 in fail case*/
int Agrumino::intRead(int address)
{
    //illegal read: trying to read the reserved addresses or a non-existent address
    if(address<USERSPACE || address>(MAX_MEMORY-1))
        return -1;
    return EEPROM.read(address);
}

float Agrumino::floatRead(int address)
{
    //illegal read: trying to read the reserved addresses or a non-existent address
    if(address<USERSPACE || address>(MAX_MEMORY-1) || (address+3)>(MAX_MEMORY-1))
        return (float) -1.0;

    //reconstructing the original float value
    union u_tag
    {
      byte b[4];
      float fval;
    } u;
    u.b[0] = EEPROM.read(address);
    u.b[1] = EEPROM.read(address+1);
    u.b[2] = EEPROM.read(address+2);
    u.b[3] = EEPROM.read(address+3);

    return u.fval;
}

char Agrumino::charRead(int address)
{
    //illegal read: trying to read the reserved addresses or a non-existent address
    if(address<USERSPACE || address>(MAX_MEMORY-1))
        return -1;

    return (char) EEPROM.read(address);
}

bool Agrumino::boolRead(int address)
{
    //illegal read: trying to read the reserved addresses or a non-existent address
    if(address<USERSPACE || address>(MAX_MEMORY-1))
        return -1;

    return (bool) EEPROM.read(address);
}

/*These functions allow the user to write in arbitrary addresses, and therefore it
  breaks the use of the LASTFREEADD information. Also the free memory check can't be
  usually made since arbitrary bytes are being written, and should correctly handled
  by the user itself*/

bool Agrumino::intArbitraryWrite(int address, int value)
{
    if(address>MAX_MEMORY-1 || address<USERSPACE)
        return false;

    int lastAvaiableAddress = getLastAvaiableAddress();
    int freeMemory = getFreeMemory();

    bool avaiability = isFree(address);
    EEPROM.write(address,value);

    //the only case in which LASTFREEADD is still useful
    if(address = lastAvaiableAddress)
    {
        EEPROM.put(LASTFREEADD,(getLastAvaiableAddress()+1));
        EEPROM.commit();
    }

    //if writing on a free address we can update the free memory information
    if(avaiability)
    {
        EEPROM.put(FREE_MEMORY,(getFreeMemory()-1));
        EEPROM.commit();
    }
    setDirty(true);

    return true;
}

bool Agrumino::floatArbitraryWrite(int address, float value)
{
    if(address>(MAX_MEMORY-1) || (address+3)>(MAX_MEMORY-1) || address<USERSPACE)
        return false;

    int lastAvaiableAddress = getLastAvaiableAddress();
    int freeMemory = getFreeMemory();

    bool avaiability = isFree(address);


    union u_tag
    {
        byte b[4];
        float fval;
    } u;

    u.fval=value;

    //note that it is not verified that the these cells aren't corrupting other datas
    EEPROM.write(address  , u.b[0]);
    EEPROM.write(address+1, u.b[1]);
    EEPROM.write(address+2, u.b[2]);
    EEPROM.write(address+3, u.b[3]);

    if(address==lastAvaiableAddress)
    {
        EEPROM.put(FREE_MEMORY,(getFreeMemory()));
        EEPROM.commit();
    }
    if(avaiability)
    {
        EEPROM.put(FREE_MEMORY,(getFreeMemory()-4));
        EEPROM.commit();
    }

    setDirty(true);
    return true;
}

bool Agrumino::charArbitraryWrite(int address, char value)
{
    if(address>MAX_MEMORY-1 || address<USERSPACE)
        return false;

    int lastAvaiableAddress = getLastAvaiableAddress();
    int freeMemory = getFreeMemory();

    bool avaiability = isFree(address);

    EEPROM.write(address,value);
    EEPROM.commit();

    if(address=lastAvaiableAddress)
    {
        EEPROM.put(LASTFREEADD,(getLastAvaiableAddress()+1));
        EEPROM.commit();
    }

    if(avaiability)
    {
        EEPROM.put(FREE_MEMORY,(getFreeMemory()-1));
        EEPROM.commit();
    }

    setDirty(true);
    return true;
}

bool Agrumino::boolArbitraryWrite(int address, bool value)
{
    if(address>MAX_MEMORY-1 || address<USERSPACE)
        return false;

    int lastAvaiableAddress = getLastAvaiableAddress();
    int freeMemory = getFreeMemory();

    bool avaiability = isFree(address);

    EEPROM.write(address,value);
    EEPROM.commit();

    if(address==lastAvaiableAddress)
    {
        EEPROM.put(LASTFREEADD,(getLastAvaiableAddress()+1));
        EEPROM.commit();
    }

    if(avaiability)
    {
        EEPROM.put(FREE_MEMORY,(getFreeMemory()-1));
        EEPROM.commit();
    }

    setDirty(true);
    return true;
}


void Agrumino::initWire() {
  Wire.begin(PIN_SDA, PIN_SCL);
}

void Agrumino::initBoard() {
  initWire();
  initLuxSensor();  // Boot time depends on the selected ADC resolution (16bit first reading after ~90ms)
  initSoilSensor(); // First reading after ~30ms
  initTempSensor(); // First reading after ~?ms
  initGpioExpander(); // First operation after ~?ms
  delay(90); // Ensure that the ICs are init properly
}

void Agrumino::setupGpioModes() {
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_BTN_S1, INPUT_PULLUP);
  pinMode(PIN_USB_DETECT, INPUT);
  pinMode(PIN_MOSFET, OUTPUT);
  pinMode(PIN_BATT_STAT, INPUT_PULLUP);
}

void Agrumino::printLogo() {
  Serial.println("\n\n\n");
  Serial.println(" ____________________________________________");
  Serial.println("/\\      _                       _            \\");
  Serial.println("\\_|    /_\\  __ _ _ _ _  _ _ __ (_)_ _  ___   |");
  Serial.println("  |   / _ \\/ _` | '_| || | '  \\| | ' \\/ _ \\  |");
  Serial.println("  |  /_/ \\_\\__, |_|  \\_,_|_|_|_|_|_||_\\___/  |");
  Serial.println("  |        |___/                  By Lifely  |");
  Serial.println("  |  ________________________________________|_");
  Serial.println("  \\_/_________________________________________/");
  Serial.println("");
}
