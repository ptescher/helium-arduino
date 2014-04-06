#ifndef DATAPACK_H
#define DATAPACK_H

#include <stdint.h>
#include <string.h>
#include "config.h"
#include "htypes.h"

/** The DataPack class is used to create a packed data block.  See DataUnpack for
the reverse process.
 */

class DataPack {
 public:
    DataPack(u8 api, u8 actionset, u8 action, u8 objcount, u16 size=DEFAULT_DP_SIZE);
    ~DataPack();

    // Functions to append data (serialize) to a datapack
    void appendArray(int count);
    void appendMap(int count);
    void appendBlock(u8 *block, u16 len);
    void appendU64(u64 n)        { doInt(0xcf, (u8*)&n, 8); }
    void appendS64(s64 n)        { doInt(0xd3, (u8*)&n, 8); }
    void appendU32(u32 n)        { doInt(0xce, (u8*)&n, 4); }
    void appendS32(s32 n)        { doInt(0xd2, (u8*)&n, 4); }
    void appendU16(u16 n)        { doInt(0xcd, (u8*)&n, 2); }
    void appendS16(s16 n)        { doInt(0xd1, (u8*)&n, 2); }
    void appendU8(u8 n);
    void appendS8(s8 n);
    void appendBool(u8 b);
    void appendString(char *s)   { appendBlock((u8*)s, strlen(s)); }
    void appendFloat(float n)    { doInt(0xca, (u8*)&n, 4); }
    // On AVR, double is really a float
    void appendDouble(double n)  { doInt(0xca, (u8*)&n, 4); }

    u8 *getBuf(void)    { return mpbuf; }
    int getBufSize(void)  { return mpbufndx; }
    u8 getSequence(void) { return sequence; }

    u8 api;
    u8 actionset;
    u8 action;

 private:
    u8 *mpbuf;
    u16 mpbuflen;
    u16 mpbufndx;
    void addByte(u8 b);
    void doInt(u8 type, u8 *bytes, u8 count);
    static u8 sequence;
};

#endif
