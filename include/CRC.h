#ifndef CRC_h
#define CRC_h

#include <inttypes.h>

typedef uint32_t CRC32;


template <typename T>
CRC32 objectCRC(T &t, bool excludeTrailingCRC = true) {
  CRC32 remainder = ~0L;
  uint8_t *ptr = (uint8_t *) &t;

  const CRC32 crcTable[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  for (int count = sizeof(T) - (excludeTrailingCRC ? sizeof(CRC32) : 0); count; --count, ptr++) {
    remainder = crcTable[(remainder ^ *ptr) & 0x0f] ^ (remainder >> 4);
    remainder = crcTable[(remainder ^ (*ptr >> 4)) & 0x0f] ^ (remainder >> 4);
    remainder = ~remainder;
  }

  return remainder;
}

#endif