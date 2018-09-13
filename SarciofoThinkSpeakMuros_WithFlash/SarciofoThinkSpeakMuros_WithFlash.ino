#include <Agrumino.h>
#include <ESP8266WiFi.h>

//////////////////////////
///     WIFI SETUP    ///
////////////////////////

#define SSID   "SSID"
#define PASSWORD    "password"

#define WIFITIMEOUT 10 /* 10 seconds timeout for connecting to wifi */

#define SLEEP_TIME_SEC 3600  

Agrumino agrumino;

int hours = 4; //change this to change the frequency of the data pushing

//temp variable to store the addresses
int attUSBADD;
int chargADD;
int buttonADD;
int tempADD;
int soilADD;
int illADD;
int voltADD;
int battLVLADD;

//tells us if the memory is dirty
boolean wrote=false;

///////////////////////////////////
///        THING SPEAK         ///
/////////////////////////////////
const char* host = "api.thingspeak.com";
const char* writeAPIKey = "Use the Key that Thingspeak indicates for writing data";


void setup() {

  Serial.begin(115200);
  Serial.println("Boot"); 
  agrumino.setup();
  agrumino.turnBoardOn();
  agrumino.enableMemory();
  
  //checking if the memory is "dirty" (A.K.A if there's some data to push)
  if(agrumino.getDirty()==0)
    wrote=false; //not dirty
  else
     wrote=true; //dirty
  blinkLed(500,2);
}

void setup_wifi() {

  int tryWiFi = WIFITIMEOUT * 2;
  delay(100);

  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  while ((WiFi.status() != WL_CONNECTED) && (tryWiFi > 0))
  {
    delay(500);
    tryWiFi --;
    Serial.print(".");
  }

  if (tryWiFi == 0)
  {
    blinkLed(300,3);
    agrumino.turnBoardOff(); // Board off before delay/sleep to save battery :)
    deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
  }
}

void loop() {

  Serial.println("#########################\n");

    if(wrote && agrumino.getHours()>=hours) //checking if enough time has been passed since the last upload
    {
      setup_wifi(); //setting up wifi only when pushing data
      int h = 0; //used to iterate
      while(h<agrumino.getHours())
      {
          //Reading all the datas from the flash
          Serial.println("("+String(h)+")");
          //getting the address of every sensor data, based on the offset of its data type
          attUSBADD = agrumino.getStartAddress();
          chargADD = attUSBADD+1 ;
          buttonADD = chargADD+1;
          tempADD = buttonADD+1;
          soilADD = tempADD+4;
          illADD = soilADD+4;
          voltADD = illADD+4;
          battLVLADD = voltADD+4;


          //printing the values obtained from the memory
          Serial.println("\nREAD FROM FLASH: ");
          Serial.println("isAttachedToUSB:   " + String(agrumino.boolRead(attUSBADD)));
          Serial.println("isBatteryCharging: " + String(agrumino.boolRead(chargADD)));
          Serial.println("isButtonPressed: "+String(agrumino.boolRead(buttonADD)));
          Serial.println("temperature:       " + String(agrumino.floatRead(tempADD)));
          Serial.println("soilMoisture:      " + String(agrumino.floatRead(soilADD)));
          Serial.println("illuminance :      " + String(agrumino.floatRead(illADD)));
          Serial.println("batteryVoltage :   " + String(agrumino.floatRead(voltADD)));
          Serial.println("batteryLVL: "+String(agrumino.intRead(battLVLADD)));
          Serial.println("");

          //putting the data into variables
          boolean isAttachedToUSB =   agrumino.boolRead(attUSBADD);
          boolean isBatteryCharging = agrumino.boolRead(chargADD);
          boolean isButtonPressed =   agrumino.boolRead(buttonADD);
          float temperature =         agrumino.floatRead(tempADD);
          float soilMoisture = agrumino.floatRead(soilADD);
          float illuminance =         agrumino.floatRead(illADD);
          float batteryVoltage =      agrumino.floatRead(voltADD);
          unsigned int batteryLevel = agrumino.intRead(battLVLADD);

          //soil moist. % calculus
          float soilMoisturePerc = (2860-soilMoisture)/14;

          /////thingspeak
          Serial.println("connecting to Thingspeak :");
          
          int tryWiFi = WIFITIMEOUT * 2;
          WiFiClient client;
          const int httpPort = 80;
          while ((!client.connect(host, httpPort)) && (tryWiFi > 0))
          {
            delay(500);
            tryWiFi --;
            Serial.print(".");
          }
          if (tryWiFi > 0) {
          String url = "/update?key=";
          url += writeAPIKey;
          url += "&field1=";
          url += String(temperature*1000);
          url += "&field2=";
          url += String((int)soilMoisturePerc);
          url += "&field3=";
          url += String(illuminance);
          url += "&field4=";
          url += String(batteryVoltage*1000);
          url += "\r\n";
        
          // Request to the server
          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                       "Host: " + host + "\r\n" +
                       "Connection: close\r\n\r\n");
          Serial.println("Sent to Thingspeak :" + url);
          blinkLed(500,2);
          } else blinkLed ( 300,4);

          agrumino.setStartAddress(battLVLADD+1); //updating the starting point for the next read
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
      boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
      boolean isBatteryCharging = agrumino.isBatteryCharging();
      boolean isButtonPressed =   agrumino.isButtonPressed();
      float temperature =         agrumino.readTempC();
      float soilMoisture = agrumino.readSoilRaw();
      float illuminance =         agrumino.readLux();
      float batteryVoltage =      agrumino.readBatteryVoltage();
      unsigned int batteryLevel = agrumino.readBatteryLevel();
    
      
      Serial.println("\nSTORING DATA IN FLASH...");

      //reading the different data, and storing them into the flash memory. Also backupping the registers of every data
      attUSBADD = agrumino.getLastAvaiableAddress();
      agrumino.boolWrite(isAttachedToUSB);
      Serial.println("wrote attacchedUSB (bool) in REG_"+String(attUSBADD));

      chargADD = agrumino.getLastAvaiableAddress();
      agrumino.boolWrite(isBatteryCharging);
      Serial.println("wrote chargBatt (bool) in REG_"+String(chargADD));

      buttonADD = agrumino.getLastAvaiableAddress();
      agrumino.boolWrite(isButtonPressed);
      Serial.println("wrote button status (bool) in REG_"+String(buttonADD));
      
      tempADD = agrumino.getLastAvaiableAddress();
      agrumino.floatWrite(temperature);
      Serial.println("wrote the temperature (float) in REG_"+String(tempADD));
      
      soilADD = agrumino.getLastAvaiableAddress();
      agrumino.floatWrite(soilMoisture);
      Serial.println("wrote the soil (int) in REG_"+String(soilADD));

      illADD = agrumino.getLastAvaiableAddress();
      agrumino.floatWrite(illuminance);
      Serial.println("wrote the ill. (float) in REG_"+String(illADD));

      voltADD = agrumino.getLastAvaiableAddress();
      agrumino.floatWrite(batteryVoltage);
      Serial.println("wrote the battery Voltage (float) in REG_"+String(voltADD));
            
      battLVLADD = agrumino.getLastAvaiableAddress();
      agrumino.intWrite(batteryLevel);
      Serial.println("wrote the battery level (int) in REG_"+String(battLVLADD));

      //increasing the hours counter
      agrumino.incrHours();
            
      //printing the values obtained from the sensors, and confronting them with the value stored in memory, to check its legality
      Serial.println("\nJUST STORED IN MEMORY: ");
      Serial.println("isAttachedToUSB:   " + String(agrumino.boolRead(attUSBADD)));
      Serial.println("isBatteryCharging: " + String(agrumino.boolRead(chargADD)));
      Serial.println("isButtonPressed: "+String(agrumino.boolRead(buttonADD)));
      Serial.println("temperature:       " + String(agrumino.floatRead(tempADD)));
      Serial.println("soilMoisture:      " + String(agrumino.floatRead(soilADD)));
      Serial.println("illuminance :      " + String(agrumino.floatRead(illADD)));
      Serial.println("batteryVoltage :   " + String(agrumino.floatRead(voltADD)));
      Serial.println("batteryLevel: "+String(agrumino.intRead(battLVLADD)));
      Serial.println("");
  }

  agrumino.turnBoardOff(); // Board off before delay/sleep to save battery :)
  deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
}

/////////////////////
// Utility methods //
/////////////////////
void blinkLed(int duration, int blinks) {
  for (int i = 0; i < blinks; i++) {
    agrumino.turnLedOn();
    delay(duration);
    agrumino.turnLedOff();
    if (i < blinks) {
      delay(duration); // Avoid delay in the latest loop ;)
    }
  }
}


void delaySec(int sec) {
  delay (sec * 1000);
}

void deepSleepSec(int sec) {
  ESP.deepSleep(sec * 1000000); // microseconds
}
