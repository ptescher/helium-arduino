#include "dataunpack.h"


#define swap16(x)  ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#define swap32(x) __builtin_bswap32(x)
#define swap64(x) __builtin_bswap64(x)

u8 DataUnpack::sequence;

DataUnpack::DataUnpack(u8 *buf, u16 len, AppData *appData)
{
    inBuf = buf;
    inBufLen = len;
    sequenceGood = 0xff; // 0xff means that we don't have a sequence yet.

    flags = appData->flags;
    if (sequenceGood == 0xff)
    {
        // New, this is first packet
        sequenceGood = 1;
        // Save this last sequence number
    sequence = appData->sequence;
    }
    else if (sequence == appData->sequence)
        // Duplicate packet, mark as bad
        sequenceGood = 0;
    else
    {
        // Good sequence number
        sequenceGood = 1;
        sequence = appData->sequence;
    }
}

/* De-serialization functions */
u8 DataUnpack::getU64(u64 *val)
{
    if (*inBuf != 0xcf)
        return 1;
    if (inBufLen < 9)
        return 1;
    inBuf++;
    *val = swap64(*((u64*)inBuf));
    inBufLen -= 9;
    inBuf += 8;
    return 0;
}

u8 DataUnpack::getS64(s64 *val)
{
    if (*inBuf != 0xd3)
        return 1;
    if (inBufLen < 9)
        return 1;
    inBuf++;
    *val = swap64(*((u64*)inBuf));
    inBufLen -= 9;
    inBuf += 8;
    return 0;
}

u8 DataUnpack::getU32(u32 *val)
{
    if (*inBuf != 0xce)
        return 1;
    if (inBufLen < 5)
        return 1;
    inBuf++;
    *val = swap32(*((u32*)inBuf));
    inBufLen -= 5;
    inBuf    += 4;
    return 0;
}

u8 DataUnpack::getS32(s32 *val)
{
    if (*inBuf != 0xd2)
        return 1;
    if (inBufLen < 5)
        return 1;
    inBuf++;
    *val = swap32(*((u32*)inBuf));
    inBufLen -= 5;
    inBuf    += 4;
    return 0;
}

u8 DataUnpack::getFloat(float *val)
{
    u32 *buf = (u32*)val;
    if (*inBuf != 0xca) return 1;
    if (inBufLen < 5) return 1;
    inBuf++;
    *buf = swap32(*((u32*)inBuf));
    inBufLen -= 5;
    inBuf += 4;
    return 0;
}

u8 DataUnpack::getU16(u16 *val)
{
    if (*inBuf != 0xcd)
        return 1;
    if (inBufLen < 3)
        return 1;
    inBuf++;
    *val = swap16(*((u16*)inBuf));
    inBufLen -= 3;
    inBuf    += 2;
    return 0;
}

u8 DataUnpack::getS16(s16 *val)
{
    if (*inBuf != 0xd1)
        return 1;
    if (inBufLen < 3)
        return 1;
    inBuf++;
    *val = swap16(*((u16*)inBuf));
    inBufLen -= 3;
    inBuf    += 2;
    return 0;
}

u8 DataUnpack::getU8(u8 *val)
{
    if (inBufLen < 1) return 1;
    if ((*inBuf & 0x80) == 0)
        // Fixed 7bit u8
        *val = (*inBuf++) & 0x7f;
    else if (*inBuf == 0xcc)
    {
        if (inBufLen < 2) return 1;
        inBuf++;
        *val = *inBuf++;
        inBufLen--;
    }
    else return 1;

    inBufLen--;
    return 0;
}

u8 DataUnpack::getS8(s8 *val)
{
    if (inBufLen < 1) return 1;
    if ((*inBuf & 0xe0) == 0xe0)
        // fixed 5-bit neg
        *val = 0 - (*inBuf++ & 0x1f);
    else if (*inBuf == 0xd0)
    {
        if (inBufLen < 2) return 1;
        inBuf++;
        *val = *(s8*)inBuf++;
        inBufLen--;
    }
    else return 1;
    inBufLen--;
    return 0;
}

u8 DataUnpack::getString(char *str, u16 len)
{
    u16 actualLen;
    if (getBlock((u8*)str,len-1, &actualLen))
        return 1;
    str[actualLen] = 0; // Asciiz terminate
    return 0;
}

u8 DataUnpack::getBlock(u8 *block, u16 maxlen, u16 *len)
{
    if (inBufLen < 2) return 1;
    if ((*inBuf & 0xe0) == 0xa0)
    {
        // Block up to 31 bytes
        u8 blen = (*(inBuf++) & 0x1f);
        if (inBufLen < (u16)blen + 1)
            return 1;
        if (blen > maxlen)
            return 1;
        memcpy(block, inBuf, blen);
        *len = blen;
        inBufLen -= 1 + (u16)blen;
        inBuf += blen;
        return 0;
    }

    if (inBufLen < 3) return 1;
    switch (*inBuf)
    {
    case 0xd9:
    case 0xc4:
        {
            // Block with one byte of length
            u8 blen = *++inBuf;
            inBuf++;
            if (inBufLen < (u16)blen + 2)
                return 1;
            if (blen > maxlen)
                return 1;
            memcpy(block, inBuf, blen);
            *len = blen;
            inBufLen -= 2 + (u16)blen;
            inBuf += blen;
        }
        break;
    case 0xda:
    case 0xc5:
        {
            // Block with two bytes length
            u16 blen = *++inBuf;
            blen <<= 8;
            blen += *(++inBuf);
            inBuf++;
            if (inBufLen < blen + 3)
                return 1;
            if (blen > maxlen)
                return 1;
            memcpy(block, inBuf, blen);
            *len = blen;
            inBufLen -= 3 + blen;
            inBuf += blen;
        }
        break;
    default:
        return 1;
    }
    return 0;
}

u8 DataUnpack::getBool(u8 *val)
{
    u8 token = *inBuf;
    if (inBufLen < 1)
        return 1;
    if ((token & 0xfe) != 0xc2)
        return 1;
    *val = token & 1;
    inBuf++;
    inBufLen--;
    return 0;
}

u8 DataUnpack::getArray(u16 *count)
{
    if (inBufLen < 1) return 1;
    if ((*inBuf  & 0xf0) == 0x90)
    {
        *count = *inBuf & 0x0f;
        inBuf++;
        inBufLen--;
        return 0;
    }
    if (inBufLen < 3) return 1;
    if (*inBuf == 0xdc)
    {
        u16 len = inBuf[1];
        len <<= 8;
        len += inBuf[2];
        *count = len;
        inBuf += 3;
        inBufLen -= 3;
        return 0;
    }
    return 1;
}

u8 DataUnpack::getMap(u16 *count)
{
    if (inBufLen < 1) return 1;
    if ((*inBuf  & 0xf0) == 0x80)
    {
        *count  = *inBuf  & 0x0f;
        inBuf++;
        inBufLen--;
        return 0;
    }
    if (inBufLen < 3) return 1;
    if (*inBuf == 0xde)
    {
        u16 len = inBuf[1];
        len <<= 8;
        len += inBuf[2];
        *count = len;
        inBuf += 3;
        inBufLen -= 3;
        return 0;
    }
    return 1;
}

// Return the type of the next object in the buffern
ObjectType DataUnpack::getNextType(void)
{
    u8 b = *inBuf;
    if (!inBufLen) return mpUnknown;

    // Examine first byte
    if (b < 0x80)           return mpU8;
    if ((b & 0xf0) == 0x80) return mpMap;
    if ((b & 0xf0) == 0x90) return mpArray;
    if ((b & 0xe0) == 0xa0) return mpString;
    if (b == 0xc2)          return mpBool;
    if (b == 0xc3)          return mpBool;
    if (b == 0xc4)          return mpArray;
    if (b == 0xc5)          return mpArray;
    if (b == 0xca)          return mpFloat;
    if (b == 0xcc)          return mpU8;
    if (b == 0xcd)          return mpU16;
    if (b == 0xce)          return mpU32;
    if (b == 0xcf)          return mpU64;
    if (b == 0xd0)          return mpS8;
    if (b == 0xd1)          return mpS16;
    if (b == 0xd2)          return mpS32;
    if (b == 0xd3)          return mpS64;
    if (b == 0xd9)          return mpString;
    if (b == 0xda)          return mpString;
    if (b == 0xdb)          return mpString;
    if (b == 0xdc)          return mpArray;
    if (b == 0xdd)          return mpArray;
    if (b == 0xde)          return mpArray;
    if ((b & 0xe0) == 0xe0) return mpU8;
    return mpUnknown;
}
