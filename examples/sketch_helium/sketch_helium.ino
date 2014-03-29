#include <helium.h>
#include <SPI.h>


/* Enable beeper on springer PCB (use beep() func to beep): */
#define SPEAKERPIN      5    // speaker connected to digital pin 9
#define SPEAKERPINOPP   6    // Invert the opposite pin to make the speaker louder
void beep (float noteFrequency, long noteDuration);


HeliumModem *modem;
int count;
u32 loopCtr;

void setup(void)
{
    // Configure both speaker pins
    pinMode(SPEAKERPIN, OUTPUT); // sets the SPEAKERPIN to be an output
    pinMode(SPEAKERPINOPP, OUTPUT); // sets the SPEAKERPIN to be an output

    modem = new HeliumModem();
    modem->sleepModem(1);

    Serial.begin(9600);
    Serial.println("Helduino Alive!");
}

void loop (void)
{
    DataUnpack *dp = modem->loop();
    if (dp)
    {
        /******  Process input data, write your code here: ******/
        Serial.print("Got something back!\n");
        u8 v;
        if (!dp->getU8(&v) &&
            v == 44)
        {
            Serial.println("** Success!!");
        }
        Serial.println(v, HEX);

        // User MUST delete the dp object:
        delete dp;
    }

    // Send stuff to helium
    if (loopCtr++ > 100000)
    {
        // Wake up modem
        modem->sleepModem(1);

        /*
    u8   getU64(u64 *val);
    u8   getS64(s64 *val);
    u8   getU32(u32 *val);
    u8   getS32(s32 *val);
    u8   getU16(u16 *val);
    u8   getS16(s16 *val);
    u8   getU8(u8 *val);
    u8   getS8(s8 *val);
    u8   getString(char *str, u16 len);
    u8   getBlock(u8 *block, u16 maxlen, u16 *len);
    u8   getBool(u8 *val);
    u8   getArray(u16 *count);
    u8   getMap(u16 *count);
        */

        // Send some data
        DataPack dp(240,0,0,1);
        dp.appendU8(44);
        modem->sendPack(&dp);

        /* char msg[20]; */
        /* sprintf(msg, "Arduino seq #%d", count++); */
        /* modem->sendDebugMsg(240, 0, 0, msg); */

        /* ModemStatus *stat; */
        /* modem->sleep(1); */
        /* stat = modem->getStatus(); */
        /* if (stat->type == MODEM_STATUS_FRAME) */
        /*     modem->sendDebugMsg(240, 0, 0, (char *)"Success - got ModemStatus Frame!"); */

        // Reset time counter
        loopCtr = 0;

        // And put modem back to sleep
        //modem->sleepModem(0);
    }
}


/* void beep (float noteFrequency, long noteDuration) */
/* { */
/*     int x; */
/*     // Convert the frequency to microseconds */
/*     float microsecondsPerWave = 1000000/noteFrequency; */
/*     // Calculate how many HIGH/LOW cycles there are per millisecond */
/*     float millisecondsPerCycle = 1000/(microsecondsPerWave * 2); */
/*     // Multiply noteDuration * number or cycles per millisecond */
/*     float loopTime = noteDuration * millisecondsPerCycle; */
/*     // Play the note for the calculated loopTime. */
/*     for (x=0;x<loopTime;x++) */
/*     { */
/*         digitalWrite(SPEAKERPIN,HIGH); */
/*         digitalWrite(SPEAKERPINOPP,LOW); */
/*         delayMicroseconds(microsecondsPerWave); */
/*         digitalWrite(SPEAKERPIN,LOW); */
/*         digitalWrite(SPEAKERPINOPP,HIGH); */
/*         delayMicroseconds(microsecondsPerWave); */
/*     } */
/* } */
