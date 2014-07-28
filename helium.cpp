#include <avr/sleep.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "helium.h"


/** @todo
    1. Add echo req/rsp commands so both ends can initiate query.
    2. Add a generic command frame so Helium can command node to sleep, take a reading, etc.
    3. Test sleeping the arduino
 */


//#define _2_SEC                   2000
#define MAX_PAYLOAD_LENGTH       64
#define WAIT_100MS               0x30000
// Set the pins used
#define SLEEP_PIN       5    // Used to  control sleeping of modem, low=sleep
#define MRESETPIN       4    // Used to reset radio modem.  Hold High
#define rxPin           6
#define txPin           7

// SPI commands (Application Node to Modem
#define CMD_GET_STATUS           0xF0   // Request modem status
#define APP_DATA_FRAME           0x82
#define MODEM_STATUS_FRAME       0x9c
#define MODEM_ECHO_FRAME         0x9d

// Structure definitions
typedef struct
{
    u32  magic;       // Magic number - sync start of header
    u16   len;         // Number of bytes of payload
} FrameHdr;

#define SOF_CHAR            (0xA3)  ///< Start-of-frame character.
#define EOF_CHAR            (0xA4)  ///< End-of-frame character.
#define ESC_CHAR            (0x1B)  ///< Escape char for byte stuffing
#define SOF_ESC             (0x01)  ///< SOF escape value
#define EOF_ESC             (0x02)  ///< EOF escape value
#define ESC_ESC             (0x03)  ///< ESC escape value

// Could use malloc() instead of static buffer
#define PP_FRAME_SIZE (1000)
static u8 ppPayload[PP_FRAME_SIZE];

// Constructor for HeliumModem object
HeliumModem::HeliumModem(void) : dpqCount(0)
{
    // Zero init stuff.
    memset(&storedModemStatus, 0, sizeof(ModemStatus));
    *(u8*)&flags = 0;

    // Setup Module Reset Pin
    pinMode(MRESETPIN, OUTPUT);
    digitalWrite(MRESETPIN, LOW);

    // Setup the Sleep pin (ATOM PCB sleeps when pin is HIGH)
    pinMode(SLEEP_PIN, OUTPUT);
    digitalWrite(SLEEP_PIN, LOW);

    // Setup serial port
    serport = new SoftwareSerial(rxPin, txPin);
    serport->begin(BAUDRATE);
}

/* The main loop for the modem object.  Must be called frequently.
   Handles UART transfers and multi-block transfers.  Returns NULL or a
   pointer to a received datapack object.
 */
void HeliumModem::loop(void)
{
    u8 ch;
    static u8  stuffed=0;
    static u16 cnt;
    static u8 *ptr;

    // Check if radio module has data for us.
    while (serport->available())
    {
        ch = serport->read();

        // See if we're starting a new frame (in case state is incorrect)
        if (ch == SOF_CHAR)
        {
            stuffed = 0;
            ppFrame.state = sof;
            if (ppFrame.payload)
            {
                //free(ppFrame.payload);
                ppFrame.payload = 0;
            }
        }

        // Check for stuffed char
        if (ch == ESC_CHAR)
        {
            // See if we have two or more ESC's in a row
            if (stuffed)
                // Already unstuffing, this is two ESC_CHAR's in a row
                // User is exiting PP mode
                return;
            // Un-stuff next char
            stuffed = 1;
            continue;
        }

        // Un-stuff the char
        if (stuffed)
        {
            switch (ch)
            {
            case SOF_ESC:
                ch = SOF_CHAR;
                break;
            case EOF_ESC:
                ch = EOF_CHAR;
                break;
            case ESC_ESC:
                ch = ESC_CHAR;
                break;
            default:
                // Very bad, wrong char, throw out everything
                if (ppFrame.payload)
                {
                    // Free the buffer
                    //free(ppFrame.payload);
                    ppFrame.payload = 0;
                }
                // Start over
                ppFrame.state = sof;
                break;
            }
            stuffed = false;
        }

        // Add a char to the frame
        switch (ppFrame.state)
        {
        case sof:
            // Waiting for sof
            if (ch == SOF_CHAR)
                ppFrame.state = sstate_t((int)ppFrame.state + 1);
            break;
        case len1:
            // length low byte
            ppFrame.length = ch;
            ppFrame.state = sstate_t((int)ppFrame.state + 1);
            break;
        case len2:
            // length high byte
            ppFrame.length += ((u16)ch) << 8;
            ppFrame.state = sstate_t((int)ppFrame.state + 1);
            // Save length to count so we can count the payload as it comes
            cnt = ppFrame.length;
            // Allocate a buffer for payload
            ptr = 0;
            if (cnt)
            {
                if (cnt > PP_FRAME_SIZE)
                    // Too big
                    ppFrame.state = sof;
                else
                    ppFrame.payload = ptr = ppPayload;;
            }
            else
                // No payload, quit now
                ppFrame.state = eof;
            break;
        case payload:
            // Save the char in buffer
            *ptr++ = ch;
            cnt--;
            if (!cnt)
                // Done with payload, move on
                ppFrame.state = sstate_t((int)ppFrame.state + 1);
            break;
        case eof:
            if (ch == EOF_CHAR)
            {
                // Process this frame in application code
                processRxData(ppFrame.payload, ppFrame.length);
                // Clear the memory pointer
                ppFrame.payload = 0;
            }
            else
            {
                // Oops, this isn't right, free the buffer
                //free(ppFrame.payload);
                ppFrame.payload = 0;
            }

            // Ready for next serial frame
            ppFrame.state = sof;
            // Prepare for next frame, buffer to be freed by stack
            ppFrame.payload = 0;
            break;
        default:
            break;
        }
    }
}

/*
    Handles the received SPI data messages from the radio module.
    Some packets are handled here, and msgpack packages are handed off
    upstream.
*/
void HeliumModem::processRxData(u8 *rxbuf, u8 len)
{
    if (!len)
        return;

    // Handle multiple transfers in one buffer
    while (len)
    {
        // Dispatch byte is first byte of buffer
        switch(*rxbuf)
        {
        case MODEM_ECHO_FRAME:
            // Got an echo back, it's awake
            flags.sleeping = 0;
            len--;
            break;
        case MODEM_STATUS_FRAME:
            // Save it away
            if (len >= sizeof(ModemStatus))
            {
                // Save the frame, set the flag that we've received data
                memcpy(&storedModemStatus, rxbuf, sizeof(ModemStatus));
                flags.gotModemStatus = 1;
                len -= sizeof(ModemStatus);
            }
            break;
        case APP_DATA_FRAME:
            {
                // Return datapack object
                AppData *hdr = (AppData*)rxbuf;
                DataUnpack *dp;
                if (len < sizeof(AppData) + hdr->length) break;
                dp = new DataUnpack(hdr->payload, hdr->length, hdr);
                if (dp->goodSeq())
                    // Sequence must increment or it's a duplicate
                addDpq(dp);
                else
                    // Not going to add dp to queue, delete here
                    delete dp;
                len -= sizeof(AppData) + hdr->length;
            }
            break;
        default:
            // No valid type found, dump the rest of the frame
            /// @todo get rid of this, syncing should be good from SPI header sync
            // len--;
            // rxbuf++;
            len = 0;
            // if (*rxbuf < 0x80)
            // {
            //     void beep (float noteFrequency, long noteDuration);
            //     beep(600, 50);
            // }
            break;
        }
    }
    return;
}

void HeliumModem::sendStuffedChar(u8 ch)
{
    if (ch == SOF_CHAR)
    {
        serport->write(ESC_CHAR);
        serport->write(SOF_ESC);
    }
    else if (ch == EOF_CHAR)
    {
        serport->write(ESC_CHAR);
        serport->write(EOF_ESC);
    }
    else if (ch == ESC_CHAR)
    {
        serport->write(ESC_CHAR);
        serport->write(ESC_ESC);
    }
    else
        serport->write(ch);
}

/* Send data block to modem

   @param hdr Pointer to first block to send
   @param hdrLen Size of header
   @param buf Pointer to second block to send
   @param bufLen Length of buf
 */
void HeliumModem::sendData(u8 *hdr, u8 hdrLen, u8 *buf, u16 bufLen)
{
    // Send via serial port
    // Serial hdr, SOF plus length (LSB first)
    serport->write((char)SOF_CHAR);
    serport->write((hdrLen + bufLen) & 0xff);
    serport->write((hdrLen + bufLen) >> 8);

    // Send hdr portion of frame
    while (hdrLen--)
        serport->write(*hdr++);
    // Send payload portion of frame
    while (bufLen--)
        serport->write(*buf++);

    // Send EOF
    serport->write((char)EOF_CHAR);
}

/* Provides current status of the modem and network,
   returns pointer to status, or NULL on error */
ModemStatus *HeliumModem::getStatus(void)
{
    u32 count;
    u8 buf;

    if (flags.sleeping)
        // return with flags = 0 (asleep, offline)
        return NULL;

    // Mark that we still need to receive ModemStatus
    flags.gotModemStatus = 0;

    // Ask host for status (use stat as temporary var)
    buf = CMD_GET_STATUS;
    sendData(&buf, 1, NULL, 0);

    // Wait for return
    for (count=0;count<WAIT_100MS;count++)
    {
        loop();

        if (flags.gotModemStatus)
            // New ModemStatus was received
            return &storedModemStatus;
    }

    // Failed to get anything back. Return as sleeping
    flags.sleeping = 1;
    return NULL;
}

// Send the buffer from msgpack object to helium
void HeliumModem::sendPack(DataPack *dp)
{
    AppData hdr;

    hdr.type = APP_DATA_FRAME;
    hdr.flags = 0;  /// @todo add a way for user to specify flags
    hdr.length    = dp->getBufSize();
    hdr.sequence  = dp->getSequence();
    sendData((u8*)&hdr, sizeof(AppData), (u8*)dp->getBuf(), hdr.length);
}

// Command the modem to sleep or wake
void HeliumModem::sleepModem(u8 wakeup)
{
    if (flags.sleeping && wakeup)
    {
        // Wake up
        digitalWrite(SLEEP_PIN, LOW);
        flags.sleeping = 0;
    }
    else if (!flags.sleeping && !wakeup)
    {
        // Put to sleep
        digitalWrite(SLEEP_PIN, HIGH);
        flags.sleeping = 1;
    }
}

// Make this node sleep for a while
void HeliumModem::sleep(u32 milliSeconds)
{

}

// Send a data pack with debug msg
void HeliumModem::sendDebugMsg(char *msg)
{
    DataPack dp(1);
    dp.appendString(msg);
    sendPack(&dp);
}

void HeliumModem::addDpq(DataUnpack *dp)
{
    // Add one element to list
    if (dpqCount >= DPQ_SIZE)
        // No more space, must drop packet
        return;
    dpq[dpqCount++] = dp;
}

DataUnpack *HeliumModem::getDataUnpack(void)
{
    // Get one element
    DataUnpack *dp;
    u8 i;
    if (!dpqCount) return NULL;
    dp = dpq[0];
    dpqCount--;
    // Shift the rest down
    for (i=0;i<DPQ_SIZE-1;i++)
        dpq[i] = dpq[i+1];
    return dp;
}
