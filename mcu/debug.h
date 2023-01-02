#ifndef DEBUG_H
#define DEBUG_H

#include "pgmspace.h"

void printHex(uint8_t v);
void printVHex(const void *data, uint8_t datasize);
void printHex(const void *data, uint8_t datasize);

struct buf
{
  buf(const void *_data, uint8_t _datasize)
  {
    data = _data;
    datasize = _datasize;
  }
  const void *data;
  uint8_t datasize;
};

void printValue(uint8_t v);
void printValue(uint16_t v);
void printValue(int16_t v);
void printValue(uint32_t v);
void printValue(int32_t v);
void printValue(uint64_t v);
void printValue(const buf &b);
void printString(const pstr *str);
void printStringColon(const pstr *str);
void printNewline();
void printChar(uint8_t c);

#if 0
#define D(_MSG)
#define D1(_MSG, A1)
#define D2(_MSG, A1, A2)
#else
#define D(_MSG) do { printString(PSTR(_MSG)); printNewline(); } while(0)
#define D1(_MSG, _A1) do { printStringColon(PSTR(_MSG)); printValue(_A1); printNewline(); } while(0)
#define D2(_MSG, _A1, _A2) do { printStringColon(PSTR(_MSG)); printValue(_A1); printValue(_A2); printNewline(); } while(0)
#define D4(_MSG, _A1, _A2, _A3, _A4) do { printStringColon(PSTR(_MSG)); printValue(_A1); printValue(_A2); printValue(_A3); printValue(_A4); printNewline(); } while(0)
#endif

#endif

