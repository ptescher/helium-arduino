#include <stdlib.h>
#include "datapack.h"


u8 DataPack::sequence;

DataPack::DataPack(u8 objcount, u16 size) :
    mpbuflen(size), mpbufndx(0)
{
    // Init the buffer
    mpbuf = (u8*)malloc(size);

    // Increment the sequence for each data packet sent
    sequence++;

    if (!mpbuf)
        return;

    // Add array for user's objects
    appendArray(objcount);
}

DataPack::~DataPack()
{
    delete mpbuf;
}

void DataPack::appendBlock(u8 *block, u16 len)
{
    // Add header (string object)
    u16 i;
    if (len <= 31)
        addByte(0xa0 + len);
    else if (len <= 256)
    {
        addByte(0xd9);
        addByte(len);
    }
    else
    {
        addByte(0xda);
        addByte(len >> 8);
        addByte(len & 0xff);
    }

    // Add data
    for (i=0;i<len;i++)
        addByte(*block++);
}

void DataPack::appendArray(int count)
{
    if (count <= 15)
        addByte(0x90 + count);
    else
    {
        addByte(0xdc);
        addByte(count >> 8);
        addByte(count & 0xff);
    }
}

void DataPack::appendMap(int count)
{
    if (count <= 15)
        addByte(0x80 + count);
    else
    {
        addByte(0xde);
        addByte(count >> 8);
        addByte(count & 0xf);
    }
}

void DataPack::doInt(u8 type, u8 *bytes, u8 count)
{
    u8 i;
    addByte(type);
    for (i=0;i<count;i++)
        // Add in reverse order (for endianness)
        addByte(bytes[count-i-1]);
}

void DataPack::appendU8(u8 n)
{
    if (n <= 127)
        addByte(n);
    else
    {
        addByte(0xcc);
        addByte(n);
    }
}

void DataPack::appendS8(s8 n)
{
    if (n < 0 && n >= -31)
        addByte(0xe0 + (-n));
    else
    {
        addByte(0xd0);
        addByte(n);
    }
}

void DataPack::appendBool(u8 b)
{
    addByte(0xc2 + (b != 0));
}

void DataPack::addByte(u8 b)
{
    // Add a byte to mpbuf, increment mpbufndx
    if (mpbufndx >= mpbuflen)
        return;
    mpbuf[mpbufndx++] = b;
}
