#ifndef HELIUM_H
#define HELIUM_H

#include <SoftwareSerial.h>
#include "datapack.h"
#include "dataunpack.h"
#include "config.h"
#include "htypes.h"

#define BAUDRATE 9600

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

typedef enum {sof, len1, len2, payload, eof} sstate_t;
typedef struct {
    sstate_t state;
    u16 length;
    u8  *payload;
} PpFrame;

class HeliumModem {
 public:
    HeliumModem(void);
    void loop(void);
    void reqStatus(void);
    ModemStatus *getStatus(void);
    void sendPack(DataPack *dp);
    void sleepModem(u8 wakeup);
    void sleep(u32 milliSeconds);
    void sendDebugMsg(char *msg);
    DataUnpack *getDataUnpack(void);
    void listen(void) { serport->listen(); }

 private:
    Flags flags;
    ModemStatus storedModemStatus;

    // Serial (UART) funcs and data
    PpFrame ppFrame;
    SoftwareSerial *serport;
    void sendData(u8 *hdr, u8 hdrLen, u8 *buf, u16 bufLen);
    void processRxData(u8 *rxbuf, u8 len);
    void sendStuffedChar(u8 ch);

    // DataUnpack queue, a list of received but not yet processed datapack received
    // From helium.  Not a circular queue, just shifted over each time
    DataUnpack *dpq[DPQ_SIZE];
    u8 dpqCount;

    //u8 dataReady(void) { return dpqCount; }
    void addDpq(DataUnpack *dp);
};

#endif
