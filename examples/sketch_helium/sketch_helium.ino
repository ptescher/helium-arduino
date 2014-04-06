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
        Serial.print("Got DataPack back, params = ");
        Serial.print(dp->api);
        Serial.print(dp->actionset);
        Serial.println(dp->action);

        for(;;)
        {
            ObjectType type = dp->getNextType();
            switch (type)
            {
            case mpUnknown:
                goto loopOut;
                break;
            case mpU8:
                {
                    u8 x;
                    dp->getU8(&x);
                    Serial.print("U8: ");
                    Serial.println(x);
                }
                break;
            case mpU16:
                {
                    u16 x;
                    dp->getU16(&x);
                    Serial.print("U16: ");
                    Serial.println(x);
                }
                break;
            case mpU32:
                {
                    u32 x;
                    dp->getU32(&x);
                    Serial.print("U32: ");
                    Serial.println(x);
                }
                break;
            case mpU64:
                {
                    u64 x;
                    dp->getU64(&x);
                    Serial.print("U64: ");
                    Serial.print((u32)(x>>32), HEX);
                    Serial.println((u32)x, HEX);
                }
                break;
            case mpS8:
                {
                    s8 x;
                    dp->getS8(&x);
                    Serial.print("S8: ");
                    Serial.println(x);
                }
                break;
            case mpS16:
                {
                    s16 x;
                    dp->getS16(&x);
                    Serial.print("S16: ");
                    Serial.println(x);
                }
                break;
            case mpS32:
                {
                    s32 x;
                    dp->getS32(&x);
                    Serial.print("S32: ");
                    Serial.println(x);
                }
                break;
            case mpS64:
                {
                    s64 x;
                    dp->getS64(&x);
                    Serial.print("S64: ");
                    Serial.print((u32)(x>>32), HEX);
                    Serial.println((u32)x, HEX);
                }
                break;
            case mpFloat:
                {
                    float x;
                    dp->getFloat(&x);
                    Serial.print("Float: ");
                    Serial.println(x);
                }
                break;
            case mpString:
                {
                    char str[60];
                    dp->getString(str, 60);
                    Serial.print("String: ");
                    Serial.println(str);
                }
                break;
            case mpBlock:
                {
                    u8 block[60];
                    u16 len;
                    u8 i;
                    dp->getBlock(block, 60, &len);
                    Serial.print("Block: ");
                    for (i=0;i<len;i++)
                        Serial.print(block[i], HEX);
                    Serial.println("\n");
                }
                break;
            case mpBool:
                {
                    u8 b;
                    dp->getBool(&b);
                    Serial.print("Bool: ");
                    Serial.println(b);
                }
                break;
            case mpArray:
                {
                    u16 c;
                    dp->getArray(&c);
                    Serial.print("Array: ");
                    Serial.println(c);
                }
                break;
            case mpMap:
                {
                    u16 c;
                    dp->getMap(&c);
                    Serial.print("Map: ");
                    Serial.println(c);
                }
                break;
            default:
                goto loopOut;
                break;
            }
        }

    loopOut:

        // User MUST delete the dp object:
        delete dp;
    }

    // Send stuff to helium
    if (loopCtr++ > 100000)
    {
        // Wake up modem
        modem->sleepModem(1);

        {
        // Send some data
        DataPack dp(240,5,6,3);
        dp.appendMap(4);
        dp.appendU64(0x1111222233334444);
        dp.appendS64(-3);
        dp.appendU32(0x55443322);
        dp.appendS32(-10);
        dp.appendU16(12345);
        dp.appendS16(-23000);
        dp.appendU8(250);
        dp.appendS8(-123);

        dp.appendArray(4);
        dp.appendBool(true);
        dp.appendBool(false);
        dp.appendString((char*)"abcXYZ!");
        dp.appendFloat(33.887766);

        u8 block[6] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15};
        dp.appendBlock(block, 6);
        modem->sendPack(&dp);
        }

        ModemStatus *stat;
        modem->sleep(1);
        stat = modem->getStatus();
        if (stat && stat->type == MODEM_STATUS_FRAME)
            modem->sendDebugMsg(240, 0, 0, (char *)"Success - got ModemStatus Frame!");

        {
        DataPack dp(240,5,6,2);
        dp.appendU16(12345);
        dp.appendU8(87);
        modem->sendPack(&dp);
        }

        // Reset time counter
        loopCtr = 0;

        // And put modem back to sleep
        //modem->sleepModem(0);
    }
}


void beep (float noteFrequency, long noteDuration)
{
    int x;
    // Convert the frequency to microseconds
    float microsecondsPerWave = 1000000/noteFrequency;
    // Calculate how many HIGH/LOW cycles there are per millisecond
    float millisecondsPerCycle = 1000/(microsecondsPerWave * 2);
    // Multiply noteDuration * number or cycles per millisecond
    float loopTime = noteDuration * millisecondsPerCycle;
    // Play the note for the calculated loopTime.
    for (x=0;x<loopTime;x++)
    {
        digitalWrite(SPEAKERPIN,HIGH);
        digitalWrite(SPEAKERPINOPP,LOW);
        delayMicroseconds(microsecondsPerWave);
        digitalWrite(SPEAKERPIN,LOW);
        digitalWrite(SPEAKERPINOPP,HIGH);
        delayMicroseconds(microsecondsPerWave);
    }
}
