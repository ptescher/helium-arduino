#include <helium.h>
#include <SPI.h>

//declare the helium modem
HeliumModem *modem;

void setup() {
  modem = new HeliumModem();
}

void loop()
{
      // send a message on a loop every five seconds
      DataPack dp(1);
      dp.appendString((char*)"Sent from my Helium Device");
      modem->sendPack(&dp);
      delay(5000);
}