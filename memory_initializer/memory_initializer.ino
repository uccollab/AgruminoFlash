#include <Agrumino.h>
Agrumino agrumino;

/*This is a simple sketch that must be made run before using any sketch
  that uses the FLASH. It initializes the user and reserved registers*/

void setup() {
  // put your setup code here, to run once:
}

void loop() {
  Serial.begin(115200);
  Serial.println("Setting Up Agrumino");
  agrumino.setup();
  Serial.println("Turning Board On");
  agrumino.turnBoardOn();

  Serial.println("Initializing FLASH");
  boolean initialized = agrumino.initializeMemory();
 if(initialized)
 {
      Serial.println("Success, you have 60 seconds to turn off the Board");
      delay(60000);
     
 }
 else
    Serial.println("Fail, retrying...");

}
