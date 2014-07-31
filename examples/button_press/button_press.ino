#include <SoftwareSerial.h>
#include <helium.h>

//declare the helium modem
HeliumModem *modem;

const int buttonPin = 2;     // the number of the pushbutton pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status

void setup() {
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);

  //initialize the modem
  modem = new HeliumModem();
}

void loop(){
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) {
      // check if the pushbutton is pressed.
      DataPack dp(1);
      dp.appendString((char*)"Button Pressed!");
      modem->sendPack(&dp);
    }
}
