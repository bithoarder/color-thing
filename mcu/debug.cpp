#include "debug.h"

#include <avr/interrupt.h>
#include <util/delay.h>

extern "C" void writeSerialByte(uint8_t);

///////////////////////////////////////////////////////////////

void printChar(uint8_t c)
{
  writeSerialByte(c);
  _delay_us(4);
}

///////////////////////////////////////////////////////////////

static char g_hex[16] PROGMEM = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void printHex(uint8_t v)
{
  printChar(pgm_read_byte(g_hex+(v>>4)));
  printChar(pgm_read_byte(g_hex+(v&0x0f)));
}

void printVHex(const void *data, uint8_t datasize)
{
  while(--datasize != 0xff)
  {
    printHex(*((const uint8_t*)data+datasize));
  }
}

void printHex(const void *data, uint8_t datasize)
{
  printChar('[');
  while(datasize--)
  {
    printHex(*((const uint8_t*)data));
    data = ((const uint8_t*)data) + 1;
    if(datasize!=0) printChar(' ');
  }
  printChar(']');
}

void printValue(uint8_t v) { printChar(' '); printVHex(&v, sizeof(v)); }
void printValue(uint16_t v) { printChar(' '); printVHex(&v, sizeof(v)); };
void printValue(int16_t v) { printChar(' '); printVHex(&v, sizeof(v)); };
void printValue(uint32_t v) { printChar(' '); printVHex(&v, sizeof(v)); }
void printValue(int32_t v) { printChar(' '); printVHex(&v, sizeof(v)); }
void printValue(uint64_t v) { printChar(' '); printVHex(&v, sizeof(v)); }
void printValue(const buf &b) { printChar(' '); printHex(b.data, b.datasize); }

void printString(const pstr *str)
{
  while(char c=pgm_read_byte(str++))
    printChar(c);
}

void printStringColon(const pstr *str)
{
  printString(str);
  printChar(':');
}

void printNewline()
{
  printChar('\n');
}

