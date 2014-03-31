#ifndef HTYPES_H
#define HTYPES_H

typedef uint8_t  u8;
typedef  int8_t  s8;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint32_t u32;
typedef  int32_t s32;
typedef uint64_t u64;
typedef  int64_t s64;

typedef struct
{
  u8  type;
  u8  flags;
  u8  length;
  u8  api;
  u8  action;
  u8  actionset;
  u8  sequence;
  u8  payload[];
}AppData;


#endif
