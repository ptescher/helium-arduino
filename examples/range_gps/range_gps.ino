#include <AdafruitGPS.h>
#include <SoftwareSerial.h>
#include <helium.h>

// Setup the GPS pins for Atom + Uno
#define GPS_PWR         19   // Set to pwr GPS unit
#define GPS_GND         18   // Set for GPS gnd pin
#define GPS_RX          17   // This is the GPS RX side (need to Transmit into this)
#define GPS_TX          16   // This is the GPS TX side (need to Receive from this)

SoftwareSerial mySerial(GPS_TX, GPS_RX);
Adafruit_GPS GPS(&mySerial);
HeliumModem *modem; // Helium Modem pointer
byte lqi = 0; // Helium LQI

void setup()
{
    Serial.begin(115200);
    Serial.println("Firing up...");

    pinMode(GPS_PWR, OUTPUT);
    digitalWrite(GPS_PWR, HIGH);
    pinMode(GPS_GND, OUTPUT);
    digitalWrite(GPS_GND, LOW);
    pinMode(GPS_RX, OUTPUT);    // Arduino TX's to GPS_RX
    digitalWrite(GPS_GND, LOW);
    pinMode(GPS_TX, INPUT);     // Arduino RX's from GPS_TX
    digitalWrite(GPS_GND, LOW);

    modem = new HeliumModem(); //setup a Helium Modem
    modem->sleepModem(1);

    GPS.begin(9600);
    GPS.sendCommand((char*)PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand((char*)PMTK_SET_NMEA_UPDATE_1HZ);
    GPS.sendCommand((char*)PGCMD_ANTENNA);

    delay(1000);
    // Ask for firmware version
    Serial.println(PMTK_Q_RELEASE);
}

uint32_t timer = millis();
void loop()
{
    DataUnpack *pack = modem->loop();
    if (pack)
    {
        Serial.print("Got a datapack from modem.\n");
        delete pack;
    }

    ModemStatus *ms = modem->getStatus();
    if (ms)
    {
        lqi = ms->lqi;
        Serial.print("Time = ");
        Serial.print(ms->time);
        Serial.print(", lqi = ");
        Serial.print(ms->lqi);
        Serial.print("\n");
    }

    if (modem->getState() > 0)
    {
        Serial.print("Modem state = %d\n");
    }

    // read data from the GPS in the 'main loop'
    /* GPS.read(); */
    /* /\* //ModemStatus *ms = modem->getStatus(); *\/ */
    /* if (GPS.newNMEAreceived()) { */
    /*     if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false */
    /*         return; */
    /* } */

    if (timer > millis()) timer = millis(); // reset timer

    if (millis() - timer > 2000) {
        timer = millis(); // reset the timer
        /* Serial.print("\nTime: "); */
        /* Serial.print(GPS.hour, DEC); Serial.print(':'); */
        /* Serial.print(GPS.minute, DEC); Serial.print(':'); */
        /* Serial.print(GPS.seconds, DEC); Serial.print('.'); */
        /* Serial.println(GPS.milliseconds); */
        /* Serial.print("Date: "); */
        /* Serial.print(GPS.day, DEC); Serial.print('/'); */
        /* Serial.print(GPS.month, DEC); Serial.print("/20"); */
        /* Serial.println(GPS.year, DEC); */
        /* Serial.print("Fix: "); Serial.print((int)GPS.fix); */
        /* Serial.print(" quality: "); Serial.println((int)GPS.fixquality); */
        // Request modemstatus
        modem->reqStatus();

	/* if (GPS.fix) */
        /* { */
        /*     Serial.println("Got a GPS fix, sending to Helium"); */
        /*     modem->sleepModem(1); // wake the modem up */
        /*     //ModemStatus *ms = modem->getStatus(); // TODO - this explodes the whole loop. why? */
        /*     //lqi = ms->lqi; */
        /*     DataPack dp(4); // stick 4 objects into the datapack */
        /*     dp.appendFloat(GPS.latitude); // pack lat */
        /*     dp.appendFloat(GPS.longitude); // pack lon */
        /*     dp.appendFloat(GPS.altitude); // pack altitude */
        /*     dp.appendU8(lqi); // pack lqi, broken */
        /*     modem->sendPack(&dp); // send that shit */
        /*     Serial.print("Location: "); */
        /*     Serial.print(GPS.latitude, 4); Serial.print(GPS.lat); */
        /*     Serial.print(", "); */
        /*     Serial.print(GPS.longitude, 4); Serial.println(GPS.lon); */
        /*     Serial.print("Speed (knots): "); Serial.println(GPS.speed); */
        /*     Serial.print("Angle: "); Serial.println(GPS.angle); */
        /*     Serial.print("Altitude: "); Serial.println(GPS.altitude); */
        /*     Serial.print("Satellites: "); Serial.println((int)GPS.satellites); */
	/* } */

    }
}
