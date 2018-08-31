/*
  AgruminoSample.ino edited version, that shows how the AgruminoFlash methods can be used in order to store the sensors data
*/

#include <Agrumino.h>

#define SLEEP_TIME_SEC 2

Agrumino agrumino;

void setup() {
  Serial.begin(115200);
  agrumino.setup();
}

void loop() {
  Serial.println("#########################\n");

  agrumino.turnBoardOn();
  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();
  boolean isButtonPressed =   agrumino.isButtonPressed();
  float temperature =         agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();
  unsigned int batteryLevel = agrumino.readBatteryLevel();


  //initializing the memory of the board
  bool initialized = agrumino.initializeMemory();
  if(initialized)
    Serial.println("Successful initialization");
  else
  {
     Serial.println("Failed initialization");
     return; 
  }

  //reading the different data, and storing some of them into the flash memory. Also backupping the registers of every data
  int tempAddress = agrumino.getLastAvaiableAddress();
  agrumino.floatWrite(temperature);
  int soilAddress = agrumino.getLastAvaiableAddress();
  agrumino.intWrite(soilMoisture);
  int illAddress = agrumino.getLastAvaiableAddress();
  agrumino.floatWrite(illuminance);
  int voltAddress = agrumino.getLastAvaiableAddress();
  agrumino.floatWrite(batteryVoltage);
  int attAddress = agrumino.getLastAvaiableAddress();
  agrumino.intWrite(isAttachedToUSB);
  int chargAddress = agrumino.getLastAvaiableAddress();
  agrumino.intWrite(isBatteryCharging);
  int countAddress = agrumino.getLastAvaiableAddress();
  int counter = 0; //I don't know the purpose of this counter yet
  agrumino.intWrite(counter);

  //printing the values obtained from the sensors, and confronting them with the value stored in memory, to check its legality
  Serial.println("");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB) + " --> memory says: "+String(agrumino.intRead(attAddress)));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging) + " --> memory says: "+String(agrumino.intRead(chargAddress)));
  Serial.println("isButtonPressed:   " + String(isButtonPressed));
  Serial.println("temperature:       " + String(temperature) + "°C" + " --> memory says: "+String(agrumino.floatRead(tempAddress)));
  Serial.println("soilMoisture:      " + String(soilMoisture) + "%" + " --> memory says:memory says: "+String(agrumino.intRead(soilAddress)));
  Serial.println("illuminance :      " + String(illuminance) + " lux" + " --> memory says: "+String(agrumino.floatRead(illAddress)));
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V" + " --> memory says: "+String(agrumino.floatRead(voltAddress)));
  Serial.println("batteryLevel :     " + String(batteryLevel) + "%");
  Serial.println("Counter value: "+String(agrumino.intRead(countAddress));
  Serial.println("");

  if (isButtonPressed) {
    agrumino.turnWateringOn();
    delay(2000);
    agrumino.turnWateringOff();
  }

  blinkLed();

  agrumino.turnBoardOff(); // Board off before delay/sleep to save battery :)

  // delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop repeatedly
  deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
}

/////////////////////
// Utility methods //
/////////////////////

void blinkLed() {
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
}

void delaySec(int sec) {
  delay (sec * 1000);
}

void deepSleepSec(int sec) {
  ESP.deepSleep(sec * 1000000); // microseconds
}
