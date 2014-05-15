#include <SPI.h>
#include <avr/sleep.h>
#include "helium.h"


/** @todo
    1. Add echo req/rsp commands so both ends can initiate query.
    2. Add a generic command frame so Helium can command node to sleep, take a reading, etc.
    3. Test sleeping the arduino
 */


#define SPI_MAGIC                0x234f6c0d
#define SPI_DUMMY                0x66
#define _2_SEC                   2000
#define MAX_PAYLOAD_LENGTH       64
#define WAIT_100MS               0x30000
// Set the pins used
#define SLEEP_PIN       2    // Used to  control sleeping of modem, low=sleep
#define MRESETPIN       7    // Used to reset radio modem.  Hold High
#define SPIIOPIN        8    // Used to see if radio module has data for us - polled.
#define SLAVESELECTPIN  10   // Set pin 10 as the slave select.

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
} SpiFrameHdr;

// Constructor for HeliumModem object
HeliumModem::HeliumModem(void) : dpqCount(0)
{
    u8 buf[3];

    // Zero init stuff.
    memset(&storedModemStatus, 0, sizeof(ModemStatus));
    *(u8*)&flags = 0;

    // Setup Module Reset Pin
    pinMode(MRESETPIN, OUTPUT);
    digitalWrite(MRESETPIN, HIGH);

    // Setup the Sleep pin
    pinMode(SLEEP_PIN, OUTPUT);
    digitalWrite(SLEEP_PIN, HIGH);

    // Setup radio module SPIO pin
    pinMode(SPIIOPIN, INPUT);
    digitalWrite(SPIIOPIN, LOW);

    // Set the SLAVESELECTPIN as an output:
    pinMode(SLAVESELECTPIN, OUTPUT);

    // Setup SPI master mode
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV16);   //Set to 1MHz for Radio communication
    SPI.begin();

    // Try to talk to transceiver and confirm SPI comm works
    buf[0] = CMD_GET_STATUS;
    spiSendData(buf, 1, NULL, 0);
}

/* The main loop for the modem object.  Must be called frequently.
   Handles SPI transfers and multi-block transfers.  Returns NULL or a
   pointer to a received datapack object.
 */
DataUnpack *HeliumModem::loop(void)
{
    // Check if radio module has data for us.
    if(digitalRead(SPIIOPIN) == 1)
    {
// void beep (float noteFrequency, long noteDuration);
//         beep(300, 50);
        // Poll SPI
        spiProcessRxData();
    }

    // Return data if any
    return getDataUnpack();
}

/*
    Handles the received SPI data messages from the radio module.
    Some packets are handled here, and msgpack packages are handed off
    upstream.
*/
void HeliumModem::spiProcessRxData(void)
{
    // Read SPI data first
    u8 *rxbuf = spiRxData;
    u16 len = spiGetData();
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

/*
    Sends data to the Helium Modem. This function prepares the SPI header
    along with the type and payload. Then it sends the data via the SPI.

    Param - hdr:        Pointer to the first block (probably header) to send.
    Param - hdrlen:     Number of bytes to send for first block.
    Param - payload:    Pointer to the data to send.
    Param - length:     Number of bytes to send for payload.

    Return:             None, radio module will respond later by setting
                        SPIIOPIN high and the polling loop will need to
                        act on it by calling spiGetData.
*/
void HeliumModem::spiSendData(u8 *hdr, u8 hdrlen, u8 *payload, u16 length)
{
    // Clear out the data variables.
    SpiFrameHdr spiHeader;
    u8 *hdrbuf = (u8*)&spiHeader;
    u16 i;
    memset(&spiHeader, 0, sizeof(SpiFrameHdr));

    // Load the header.
    spiHeader.magic = SPI_MAGIC;
    spiHeader.len = length + hdrlen;

    // Start SPI transmission.
    digitalWrite(SLAVESELECTPIN,LOW);

    // Need a delay for the slave to get ready.
    delayMicroseconds(200);

    // Now (this is tricky) check to see if the slave has data for us.
    // If so, handle his transfer(s) first, then resume ours
    while (digitalRead(SPIIOPIN) == 1)
    {
        // Go get Rx Data, which will reset SPIIOPIN
        spiProcessRxData();
        // Re-start SPI transmission.
        digitalWrite(SLAVESELECTPIN,LOW);
        // Need a delay for the slave to get ready.
        delayMicroseconds(200);
    }

    // Send the spi header
    for (i=0;i<sizeof(SpiFrameHdr);i++)
        SPI.transfer(*hdrbuf++);

    // Send the first payload (another header)
    for(i=0; i<hdrlen; i++)
        SPI.transfer(hdr[i]);

    // Send the payload.
    for(i=0; i<length; i++)
        SPI.transfer(payload[i]);

    // End SPI transmission.
    digitalWrite(SLAVESELECTPIN,HIGH);
}

/**
    This function is called when SPIIOPIN is detected high in the polling function.

    Param - None:   The global array spiRxData is filled with data received
                    from the radio.

    Return - None:  The function will return if the magic number or the
                    length are either incorrect or the message is too long.
                    In either case the spiRxData array will be empty.
*/
u16 HeliumModem::spiGetData(void)
{
    // Get the spi header data in two stages - magic number first (4 bytes)
    // then the 2 byte length if the magic number is right..
    SpiFrameHdr spiHeader;
    u16 i;

    spiHeader.len = 0;

    // Start SPI transmission.
    digitalWrite(SLAVESELECTPIN,LOW);

    // Make sure the received array is empty.
    memset(spiRxData, 0, MAX_DATA_LEN);

    // Need a delay for the slave to get ready.
    delayMicroseconds(200);

    // Shift in the received magic number. We get the LSB first.
    while ((i = SPI.transfer(SPI_DUMMY)) != (SPI_MAGIC & 0xff))
        if(digitalRead(SPIIOPIN) != 1)
            goto spiOut;

    spiHeader.magic = (u32)i;
    spiHeader.magic |= ((u32)SPI.transfer(SPI_DUMMY) << 8);
    spiHeader.magic |= ((u32)SPI.transfer(SPI_DUMMY) << 16);
    spiHeader.magic |= ((u32)SPI.transfer(SPI_DUMMY) << 24);

    if(SPI_MAGIC == spiHeader.magic)
    {
       // Get the length
        spiHeader.len = SPI.transfer(SPI_DUMMY);
        spiHeader.len |= ((u32)SPI.transfer(SPI_DUMMY) << 8);

        // Get the rest of the data - byte [0] is the cmd.
        if(spiHeader.len > MAX_DATA_LEN)
        {
            // Length too long, error.
            spiHeader.len = 0;
            goto spiOut;
        }

        // All clear, get the payload.
        for(i=0; i<spiHeader.len; i++)
            spiRxData[i] = SPI.transfer(SPI_DUMMY);
    }
 spiOut:
    digitalWrite(SLAVESELECTPIN,HIGH);
    return spiHeader.len;
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
    spiSendData(&buf, 1, NULL, 0);

    // Wait for return
    for (count=0;count<WAIT_100MS;count++)
    {
        // Poll the SPI port
        if(digitalRead(SPIIOPIN) == 1)
        // Poll SPI
            spiProcessRxData();

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
    spiSendData((u8*)&hdr, sizeof(AppData), (u8*)dp->getBuf(), hdr.length);
}

// Command the modem to sleep or wake
void HeliumModem::sleepModem(u8 wakeup)
{
    if (flags.sleeping && wakeup)
    {
        // Wake up
        digitalWrite(SLEEP_PIN, HIGH);
        flags.sleeping = 0;
    }
    else if (!flags.sleeping && !wakeup)
    {
        // Put to sleep
        digitalWrite(SLEEP_PIN, LOW);
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
