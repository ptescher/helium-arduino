#ifndef DATAUNPACK_H
#define DATAUNPACK_H

#include <stdint.h>
#include <string.h>
#include "htypes.h"

/** The DataUnpack class is used to extract data from a packed message.  See DataPack for
the reverse process.
 */


typedef enum {
    mpUnknown,
    mpU8,
    mpU16,
    mpU32,
    mpU64,
    mpS8,
    mpS16,
    mpS32,
    mpS64,
    mpFloat,
    mpString,
    mpBlock,
    mpBool,
    mpArray,
    mpMap,
    mpNil,
} ObjectType;

class DataUnpack {
 public:
    DataUnpack(u8 *buf, u16 len, AppData *appData);

    u8 goodSeq(void) { return sequenceGood; }

    // Functions to extract data (de-serialize) from a datapack.
    // These functions return non-zero on failure
    u8   getU64(u64 *val);
    u8   getS64(s64 *val);
    u8   getU32(u32 *val);
    u8   getS32(s32 *val);
    u8   getU16(u16 *val);
    u8   getS16(s16 *val);
    u8   getU8(u8 *val);
    u8   getS8(s8 *val);
    u8   getFloat(float *val);
    u8   getString(char *str, u16 len);
    u8   getBlock(u8 *block, u16 maxlen, u16 *len);
    u8   getBool(u8 *val);
    u8   getArray(u16 *count);
    u8   getMap(u16 *count);
    ObjectType getNextType(void);

    // Metadata vars from sender's AppData frame
    u8  flags;

 private:
    u8 *inBuf;
    u16 inBufLen;
    u8 sequenceGood;
    static u8 sequence;
};

#endif
