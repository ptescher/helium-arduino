#ifndef HELIUM_H
#define HELIUM_H

#include "datapack.h"
#include "dataunpack.h"
#include "config.h"
#include "htypes.h"

typedef struct {
    u8  type;     // MODEM_STATUS_FRAME
    u8  flags;    // Flags, b0=awake, b1=online
    u8  lqi;      // Average LQI of RF link
    u8  rate;     // Link speed, in kbits/sec
    s8  timeZone; // Local timezone, hours offset from UTC
    u32 time;     // Current time, seconds since 1/1/1970 UTC
} ModemStatus;

#define MODEM_STATUS_FRAME 0x9c

typedef struct {
    u8 sleeping:1;
    u8 gotModemStatus:1;
} Flags;

class HeliumModem {
 public:
    HeliumModem(void);
    DataUnpack *loop(void);
    ModemStatus *getStatus(void);
    void sendPack(DataPack *dp);
    void sleepModem(u8 wakeup);
    void sleep(u32 milliSeconds);
    void sendDebugMsg(char *msg);

 private:
    Flags flags;
    ModemStatus storedModemStatus;

    void spiProcessRxData(void);
    u16  spiGetData(void);
    void spiSendData(u8 *hdr, u8 hdrlen, u8 *payload, u16 length);
    unsigned char spiRxData[MAX_DATA_LEN];

    // DataUnpack queue, a list of received but not yet processed datapack received
    // From helium.  Not a circular queue, just shifted over each time
    DataUnpack *dpq[DPQ_SIZE];
    u8 dpqCount;

    //u8 dataReady(void) { return dpqCount; }
    void addDpq(DataUnpack *dp);
    DataUnpack *getDataUnpack(void);
};

#endif
