#include <helium.h>
#include <SPI.h>

//declare the helium modem
HeliumModem *modem;
byte API       = 120;
byte ACTIONSET = 122;
byte AS        = 0;
byte SEND_SMS  = 1;

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
      DataPack dp(API,AS,SEND_SMS,2);
      dp.appendString((char*)"+16178200563");
      dp.appendString((char*)"Hey dude, running late!");
      modem->sendPack(&dp);
    }
}
