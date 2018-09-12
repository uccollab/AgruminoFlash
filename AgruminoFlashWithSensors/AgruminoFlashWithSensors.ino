/*
This sketch runs infinitely and does the following steps:
1) checks if the flash memory of Agrumino is "dirty" (this mean we need to push
   some data).
2a) if the memory is dirty and the specified number of hours haven't passed 
    then the sensors data are collected and stored in flash
2b) if the specified amount of hours is matched then the data are retrieved
    from the flash memory and the memory is cleaned

*/

#include <Agrumino.h>
Agrumino agrumino;

int hours = 4; //change this to change the frequency of the data pushing
int sleepTime = 10; //time for deepsleep in seconds: change to rest more or less

//temp variable to store the addresses
int tempADD;
int soilADD;
int illADD;
int voltADD;
int attUSBADD;
int chargADD;

//tells us if the memory is dirty
boolean wrote=false;

void setup() 
{
  //initializing and powering the board, then enabling the memory
  Serial.begin(115200);
  agrumino.setup();
  agrumino.turnBoardOn();
  agrumino.enableMemory();
  
  //checking if the memory is "dirty" (A.K.A if there's some data to push)
  if(agrumino.getDirty()==0)
    wrote=false; //not dirty
  else
     wrote=true; //dirty

}

void loop() 
{   
  if(wrote && agrumino.getHours()>=hours) //checking if enough time has been passed since the last upload
  {
      int h = 0; //used to iterate
      while(h<agrumino.getHours())
      {
          //Reading all the datas from the flash
          Serial.println("("+String(h)+")");
          //getting the address of every sensor data, based on the offset of its data type
          tempADD = agrumino.getStartAddress();
          soilADD = tempADD+4;
          illADD = soilADD+1;
          voltADD = illADD+4;
          attUSBADD = voltADD+4;
          chargADD = attUSBADD+1;

          //printing the values obtained from the memory
          Serial.println("\nREAD FROM FLASH: ");
          Serial.println("isAttachedToUSB:   " + String(agrumino.boolRead(attUSBADD)));
          Serial.println("isBatteryCharging: " + String(agrumino.boolRead(chargADD)));
          Serial.println("temperature:       " + String(agrumino.floatRead(tempADD)));
          Serial.println("soilMoisture:      " + String(agrumino.intRead(soilADD)));
          Serial.println("illuminance :      " + String(agrumino.floatRead(illADD)));
          Serial.println("batteryVoltage :   " + String(agrumino.floatRead(voltADD)));
          Serial.println("");

          agrumino.setStartAddress(chargADD+1); //updating the starting point for the next read
          h++;
      }

      //cleaning the memory at the end
      boolean init = agrumino.initializeMemory();
      if(!init)
      {
        Serial.println("Final memory clean failed: aborting");
        return;
      }      
  }
  else
  {
      //collecting sensors data
      Serial.println("\nREADING DATA...");
      boolean isAttachedToUSB = agrumino.isAttachedToUSB();
      boolean isBatteryCharging = agrumino.isBatteryCharging();
      float temperature = agrumino.readTempC();
      unsigned int soilMoisture = agrumino.readSoil();
      float illuminance = agrumino.readLux();
      float batteryVoltage = agrumino.readBatteryVoltage();
    
      
      Serial.println("\nSTORING DATA IN FLASH...");
      
      //reading the different data, and storing them into the flash memory. Also backupping the registers of every data
      tempADD = agrumino.getLastAvaiableAddress();
      agrumino.floatWrite(temperature);
      Serial.println("wrote the temperature (float) in REG_"+String(tempADD));
      
      soilADD = agrumino.getLastAvaiableAddress();
      agrumino.intWrite(soilMoisture);
      Serial.println("wrote the moist (int) in REG_"+String(soilADD));

      illADD = agrumino.getLastAvaiableAddress();
      agrumino.floatWrite(illuminance);
      Serial.println("wrote the ill. (float) in REG_"+String(illADD));

      voltADD = agrumino.getLastAvaiableAddress();
      agrumino.floatWrite(batteryVoltage);
      Serial.println("wrote the battery Voltage (float) in REG_"+String(voltADD));

      attUSBADD = agrumino.getLastAvaiableAddress();
      agrumino.boolWrite(isAttachedToUSB);
      Serial.println("wrote attacchedUSB (bool) in REG_"+String(attUSBADD));

      chargADD = agrumino.getLastAvaiableAddress();
      agrumino.boolWrite(isBatteryCharging);
      Serial.println("wrote chargBatt (bool) in REG_"+String(chargADD));

      //increasing the hours counter
      agrumino.incrHours();
            
      //printing the values obtained from the sensors, and confronting them with the value stored in memory, to check its legality
      Serial.println("\nJUST STORED IN MEMORY: ");
      Serial.println("isAttachedToUSB:   " + String(agrumino.boolRead(attUSBADD)));
      Serial.println("isBatteryCharging: " + String(agrumino.boolRead(chargADD)));
      Serial.println("temperature:       " + String(agrumino.floatRead(tempADD)));
      Serial.println("soilMoisture:      " + String(agrumino.intRead(soilADD)));
      Serial.println("illuminance :      " + String(agrumino.floatRead(illADD)));
      Serial.println("batteryVoltage :   " + String(agrumino.floatRead(voltADD)));
      Serial.println("");
  }

  Serial.println("Sleeping...");
  agrumino.turnBoardOff();
  deepSleepSec(sleepTime); //in order to avoid too long I reduced the sleep timer to 10 seconds       
  blinkLed();
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
