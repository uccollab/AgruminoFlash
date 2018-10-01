/*
  Edited AgruminoSample made to demonstrate the functionalities of the flash writing/reading
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

  //starting from scratch, initializing the flash
  bool initialized = agrumino.initializeMemory();
  
  if(initialized)
    Serial.println("Successful initialization :)");
  else
    Serial.println("Failed initialization :(");

  //printing various stats
  Serial.println("MAX MEMORY: "+String(agrumino.getMaxMemory())+"B");
  Serial.println("DIRTY? -> "+String(agrumino.getDirty()));
  Serial.println("FREE MEMORY: "+String(agrumino.getFreeMemory())+"B");
  Serial.println("LAST FREE ADDRESS: "+String(agrumino.getLastAvaiableAddress()));

  //a little wait to read the stats from the monitor
  delay(10000);

  /*EXAMPLE 1: writing 10 integers into the memory, sequentially*/
  //backupping the register from which the integers starts
  int beginInts = agrumino.getLastAvaiableAddress();

  //writing 10 ints sequentially, and printing stats meanwhile
  for(int i=0; i<10; i++)
  {
    Serial.println("-------------------");
    Serial.println("WRITING: "+String(i)+" AT INDEX "+String(agrumino.getLastAvaiableAddress()));
    Serial.println("SUCCESSFUL (1=YES 0=NO): "+String(agrumino.intWrite(i)));
    Serial.println("LAST FREE ADDRESS: "+String(agrumino.getLastAvaiableAddress()));
    Serial.println("FREE MEMORY: "+String(agrumino.getFreeMemory())+"B");
    delay(3000);
  }
  Serial.println("-------------------");
  //the register of the last int
  int endInts = agrumino.getLastAvaiableAddress()-1;

  //printing the datas
  for(int i=beginInts; i<=endInts; i++)
    Serial.println("data at "+String(i)+": "+String(agrumino.intRead(i)));
  Serial.println("------------------------");
  
  //let's restart
  Serial.println("Re-cleaning the memory");
  initialized = agrumino.initializeMemory();

  /*Saving 2 ints, 4 floats, 2 bools and a char, then reading them*/
  beginInts = agrumino.getLastAvaiableAddress();
  for(int i=0; i<2; i++)
    agrumino.intWrite(12);
  endInts = agrumino.getLastAvaiableAddress()-1;
  
  int beginFloats = agrumino.getLastAvaiableAddress();
  for(int i=0; i<4; i++)
    agrumino.floatWrite(1.3);
  int endFloats = agrumino.getLastAvaiableAddress()-1; //note that for every float there's a 4B jump

  int beginBool = agrumino.getLastAvaiableAddress();
  for(int i=0; i<2; i++)
    agrumino.boolWrite(true);
  int endBool = agrumino.getLastAvaiableAddress()-1; //note that for every float there's a 4B jump

  int beginChars = agrumino.getLastAvaiableAddress();
  for(int i=0; i<1; i++)
    agrumino.charWrite(1.3);
  int endChars = agrumino.getLastAvaiableAddress()-1; //note that for every float there's a 4B jump

  Serial.println("Wrote 2 ints, 4 floats, 2 booleans, 1 char = 1*2 + 4*4 + 1*2 + 1 = 21B of data");
  delay(5000);
  Serial.println("If it is all right, then I should have jumped from address "+String(beginInts)+" to address "+String(beginInts+2+(4*4)+(2)+1)+" ---> "+" LAST FREE ADD: "+String(agrumino.getLastAvaiableAddress()));

  //printing the floats
  Serial.println("Printing the floats: ");
  for(int i=beginFloats; i<endFloats; i+=4)
    Serial.println(String(agrumino.floatRead(i)));

  //casual access: overriding the second integer with a char
  Serial.println("Overriding the second int I wrote with 'c' char.");
  agrumino.charArbitraryWrite(beginInts+1,'c');
  Serial.println("Printing the char --> "+String(agrumino.charRead(beginInts+1)));

  //now freeing up that address
  Serial.println("Freeing that address");
  agrumino.free(beginChars+1,0);

  //and checking if it worked!
  Serial.println("Is it free? (1 if yes) -->"+String(agrumino.isFree(beginChars+1)));

  //one minute just to read everything
  delay(60000);

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
