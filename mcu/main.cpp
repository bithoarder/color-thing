#include "debug.h"

extern "C"
{
  #include "usbdrv.h"
  #include "osccal.h"
}

#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define STATIC_ASSERT(condition)  extern void static_assert_test(int arg[(condition) ? 1 : -1])

///////////////////////////////////////////////////////////////////////////////

//  dw |1u8| vcc
// pb3 |2 7| pb2
// pb4 |3 6| pb1
// gnd |4 5| pb0

//         oc:01
// 1 dw        _ :
// 2 pb3       B : D+  (green)
// 3 pb4       B : red
// 4 gnd       _ :
// 5 pb0      AA : green
// 6 pb1      BA : blue
// 7 pb2/int0    : D-  (white)

// 8 vcc         :

#define LED_RED 4 // oc1a
#define LED_GREEN 0 // 0c0a 0c1a
#define LED_BLUE 1 // oc0b oc1a
#define LED_RED_MASK _BV(LED_RED)
#define LED_GREEN_MASK _BV(LED_GREEN)
#define LED_BLUE_MASK _BV(LED_BLUE)

///////////////////////////////////////////////////////////////////////////////

#define set_bit_0_if_gt(RESULT, VAL, CMP) \
  __asm__ (\
    "cp %1, %2\n\t"\
    "rol %0\n\t"\
    : "=&r"(RESULT)\
    : "0"(RESULT),\
      "r"(CMP),\
      "r"(VAL))

#define pgm_read_byte_and_increment(RESULT, ADDR) \
  __asm__ (\
    "lpm %0, %a1+\n\t"\
    : "=r"(RESULT),\
      "=z"(ADDR)\
    : "1"(ADDR))

#define pgm_read_word_and_increment(RESULT, ADDR) \
  __asm__ (\
    "lpm %A0, %a1+\n\t"\
    "lpm %B0, %a1+\n\t"\
    : "=r"(RESULT),\
      "=z"(ADDR)\
    : "1"(ADDR))

uint16_t mul(uint8_t v1, uint8_t v2)
{
  uint16_t v2l = v2;
  uint16_t res = 0;
  while(v1)
  {
    if(v1&1) res += v2l;
    v1 >>= 1;
    v2l <<= 1;
  }
  return res;
}

#if 0
#define mul(a1, a2)\
  (__extension__({\
    uint16_t result;\
    __asm__ (\
      "mul %0, %1\n\t"\
      : "=0"(result),\
      : "r"(a1),\
        "r"(a2)\
      );\
      result; }))
#endif

///////////////////////////////////////////////////////////////////////////////

static struct { uint8_t cnt; uint16_t step; } __attribute__((packed)) delaytab[] PROGMEM = {
  {11,    10}, //      0 -    100
  {18,    50}, //    150 -   1000
  {10,   200}, //   1200 -   3000
  { 4,   500}, //   3500 -   5000
  { 5,  1000}, //   6000 -  10000
  { 5,  2000}, //  12000 -  20000
  { 2,  5000}, //  25000 -  30000
  { 3, 10000}, //  40000 -  60000
  { 4, 30000}, //  90000 - 180000
  { 2, 60000}, // 240000 - 300000
};

uint32_t decodeDelay(uint8_t delaycode)
{
  uint32_t delay = 0;
  uint16_t delaytab_pm = (uint16_t)delaytab;
  for(uint8_t i=0; i<sizeof(delaytab)/sizeof(delaytab[0]); i++)
  {
    uint8_t cnt; pgm_read_byte_and_increment(cnt, delaytab_pm);
    uint16_t step; pgm_read_word_and_increment(step, delaytab_pm);
    while(cnt-- && delaycode--)
    {
      delay += step;
    }
    if(delaycode == 0)
      break;
  }
  return delay;
}

static uint8_t gammatab[] PROGMEM = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
    3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,
    6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
   12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,
   20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,
   30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,
   42,  43,  43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,
   56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
   73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  87,  88,  89,  90,
   91,  93,  94,  95,  97,  98,  99, 100, 102, 103, 105, 106, 107, 109, 110, 111,
  113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135,
  137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161,
  163, 165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190,
  192, 194, 196, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
  223, 225, 227, 229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255
};

uint8_t ycbcr2r(uint8_t y, uint8_t cb, uint8_t cr)
{
  uint16_t r1 = (y*130)<<1;
  uint16_t r2 = (cb*22)<<0;
  uint16_t r3 = (cr*183)<<3;
  int8_t r = (r1+r2+r3-10851)>>8;
  return r<0 ? 0 : r>63 ? 255 : r*4;
}

uint8_t ycbcr2g(uint8_t y, uint8_t cb, uint8_t cr)
{
  uint16_t r1 = (y*132)<<1;
  uint16_t r2 = (cb*167)<<1;
  uint16_t r3 = (cr*179)<<2;
  int8_t r = (r1-r2-r3+7882)>>8;
  return r<0 ? 0 : r>63 ? 255 : r*4;
}

uint8_t ycbcr2b(uint8_t y, uint8_t cb, uint8_t cr)
{
  uint16_t r1 = (y*136)<<1;
  uint16_t r2 = (cb*234)<<3;
  uint16_t r3 = (cr*6)<<0;
  int8_t r = (r1+r2+r3-14338)>>8;
  return r<0 ? 0 : r>63 ? 255 : r*4;
}

///////////////////////////////////////////////////////////////////////////////


uint8_t g_red=0, g_green=0, g_blue=0;
uint8_t g_cycle = 0;

// ISR(TIMER0_OVF_vect)
// {
// #if 1
//   // 17 instructins, 17-20 cycles
//   uint8_t cycle = g_cycle;
//   PORTB &= ~LED_RED_MASK;
//   if(cycle < g_red) PORTB |= LED_RED_MASK;
//   PORTB &= ~LED_GREEN_MASK;
//   if(cycle < g_green) PORTB |= LED_GREEN_MASK;
//   PORTB &= ~LED_BLUE_MASK;
//   if(cycle < g_blue) PORTB |= LED_BLUE_MASK;
// #else
//   // 18 instructins, 18 cycles
//   STATIC_ASSERT(LED_RED == 4);
//   STATIC_ASSERT(LED_GREEN == 1);
//   STATIC_ASSERT(LED_BLUE == 3);

//   uint8_t col = 0;
//   set_bit_0_if_gt(col, g_cycle, g_red); // -> 0 0 0 0 0 0 red
//   set_bit_0_if_gt(col, g_cycle, g_blue); // -> 0 0 0 0 0 0 red blue
//   col <<= 1; // -> 0 0 0 0 0 red blue 0
//   set_bit_0_if_gt(col, g_cycle, g_green); // -> 0 0 0 0 red blue 0 green
//   col <<= 1; // -> 0 0 0 red blue 0 green 0

//   // note: not atomic,
//   PORTB = (PORTB&~(LED_RED_MASK | LED_GREEN_MASK | LED_BLUE_MASK)) | col;
// #endif
//   g_cycle = (g_cycle+1);
// }

// void setColor(uint8_t y, uint8_t cb, uint8_t cr)
// {
//   uint8_t r = ycbcr2r(y, cb, cr);
//   uint8_t g = ycbcr2g(y, cb, cr);
//   uint8_t b = ycbcr2b(y, cb, cr);
//   OCR0A = 255-g;
//   OCR0B = 255-b;
//   OCR1B = r;
// }


// void cycleColors()
// {
//   uint8_t c[2][3] = {{0,0,0}, {0,0,0}};
//   for(;;)
//   {
//     c[0][0] = c[1][0];
//     c[0][1] = c[1][1];
//     c[0][2] = c[1][2];
//     c[1][0] = rand()&15;
//     c[1][1] = rand()&15;
//     c[1][2] = (rand()&31) + 32;
//     for(int i=0; i<128; i++)
//     {
//       setColor(c[0][2]+(((c[1][2]-c[0][2])*i)>>7), c[0][0]+(((c[1][0]-c[0][0])*i)>>7), c[0][1]+(((c[1][1]-c[0][1])*i)>>7));
//       _delay_ms(10);
//     }
//     // for(uint8_t i=0; i<64; i++)
//     // {
//     //   setColor(i, 15,6);
//     //   _delay_ms(100);
//     // }


//     // for(uint8_t i=0; i<255; i++)
//     // {
//     //   setColor(0, i, 0);
//     // }
//     // for(uint8_t i=0; i<255; i++)
//     // {
//     //   setColor(0, 0, i);
//     // }
//     // for(uint8_t i=0; i<255; i++)
//     // {
//     //   setColor(i, i, i);
//     // }
//   }
// }

void setupTimers()
{
  DDRB |= LED_RED_MASK | LED_GREEN_MASK | LED_BLUE_MASK;

  // set up timer0; fast pwm, clk/64 (~1007 Hz)
  TCCR0A =
    _BV(COM0A1) |
    _BV(COM0A0) |
    _BV(COM0B1) |
    _BV(COM0B0) |
    _BV(WGM01) |
    _BV(WGM00) |
    0;
  TCCR0B =
    //_BV(FOC0A) |
    //_BV(FOC0B) |
    //_BV(WGM02) |
    //_BV(CS02) |
    _BV(CS01) |
    _BV(CS00) |
    0;
  OCR0A = 255-0; // green
  OCR0B = 255-0; // blue
  TIMSK = (TIMSK&~(_BV(OCIE0A) | _BV(OCIE0B) | _BV(TOIE0))) |
    //_BV(OCIE0A) |
    //_BV(OCIE0B) |
    //_BV(TOIE0) |
    0;
  TIFR = (TIFR&~(_BV(OCF0A) | _BV(OCF0B) | _BV(TOV0))) |
    //_BV(OCF0A) |
    //_BV(OCF0B) |
    //_BV(TOV0) |
    0;

  // timer1:
  TCCR1 =
    //_BV(CTC1) |
    //_BV(PWM1A) |
    //_BV(COM1A1) |
    //_BV(COM1A0) |
    //_BV(CS13) |
    _BV(CS12) |
    _BV(CS11) |
    _BV(CS10) |
    0;

  GTCCR =  (GTCCR&~(_BV(PWM1B) | _BV(COM1B1) | _BV(COM1B0) |_BV(FOC1B) |_BV(FOC1A) | _BV(PSR1))) |
    _BV(PWM1B) |
    _BV(COM1B1) |
    //_BV(COM1B0) |
    //_BV(FOC1B) |
    //_BV(FOC1A) |
    //_BV(PSR1 ) |
    0;

  OCR1B = 0; // red
  OCR1C = 255;

  TIMSK = (TIMSK&~(_BV(OCIE1A) | _BV(OCIE1B) | _BV(TOIE1))) |
    //_BV(OCIE1A) |
    //_BV(OCIE1B) |
    //_BV(TOIE1) |
    0;

  TIFR = (TIFR&~(_BV(OCF1A) | _BV(OCF1B) | _BV(TOV1))) |
    //_BV(OCF1A) |
    //_BV(OCF1B) |
    //_BV(TOV1) |
    0;

  // PLLCSR =
  //   //_BV(LSM) |
  //   //_BV(PCKE) |
  //   //_BV(PLLE) |
  //   //_BV(PLOCK) |
  //   0;
}

void setColor(uint8_t r, uint8_t g, uint8_t b)
{
  OCR0A = 255-pgm_read_byte(gammatab + g);
  OCR0B = 255-pgm_read_byte(gammatab + b);
  OCR1B = pgm_read_byte(gammatab + r);
}

//uint8_t g_usbData[16];

#define LOAD_RGB(r,g,b) (0x0000 | ((r)<<10) | ((g)<<5) | ((b)<<0))
#define LOAD_DELAY(delay) (0x8000 | ((delay)<1024 ? (delay)<<4 : ((delay>>4)<<4)|4)) // max 32 s
#define GO 0xffd0
#define JUMP(addr) (0xe100 | (addr))

uint8_t lightprogUploadPos = 0;
uint8_t lightprogUploadSize = 0;

uint16_t lightprog[64] = {
  LOAD_DELAY(1000),
  LOAD_RGB(0, 0, 0),
  GO,
  LOAD_RGB(0, 31, 0),
  GO,
  JUMP(0),
};


usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
  usbRequest_t *rq = (usbRequest_t*)data;
  //usbMsgPtr = (uint8_t*)&g_usbData;

  switch(rq->bRequest)
  {
    case 0:
      //setColor(rq->wValue.bytes[0], rq->wValue.bytes[1], rq->wIndex.bytes[0]);
      //g_usbData[0] = 0;
      lightprogUploadPos = 0;
      lightprogUploadSize = rq->wLength.word;
      //if(lightprogUploadSize > sizeof(lightprog))
      //  lightprogUploadSize = sizeof(lightprog);
      return USB_NO_MSG;
  }

   return 0;
}

uint8_t usbFunctionWrite(uint8_t *data, uint8_t len)
{
  if(len > lightprogUploadSize)
    len = lightprogUploadSize;
  memcpy((uint8_t*)lightprog+lightprogUploadPos, data, len);

  lightprogUploadPos += len;
  lightprogUploadSize -= len;

  return lightprogUploadSize==0; // return true when done
}

uint8_t blend(uint8_t src, uint8_t dst, uint8_t alpha)
{
  return (src*(255-alpha) + dst*alpha)>>8;
}

void runLightVM()
{
  uint8_t cur_col[3] = {0, 0, 0};

  uint8_t reg_pc = 0;
  uint8_t reg_col[3] = {0, 0, 0};
  uint32_t reg_delay = 0;

  for(;;)
  {
    if(lightprogUploadSize)
    {
      reg_pc = 0; // fixme: no guarantee that we will catch the upload and reset pc
      usbPoll();
      _delay_ms(1);
      continue;
    }

    uint16_t opcode = lightprog[reg_pc++];
    if((opcode&0x8000) == 0x0000)
    {
      // 0 rrrrrgggggbbbbb - load_rgb
      reg_col[0] = (opcode>>7)&0xf8; reg_col[0] |= reg_col[0]>>3;
      reg_col[1] = (opcode>>2)&0xf8; reg_col[1] |= reg_col[1]>>3;
      reg_col[2] = (opcode<<3);      reg_col[2] |= reg_col[2]>>3;
    }
    else if((opcode&0xc000) == 0x8000)
    {
      // 10 mmmmmmmmmmeeee - load_delay # ms. max:1024<<15 = 33554432 = ~9.3h
      reg_delay = ((opcode&0x3ff0)>>4)<<(opcode&0x000f);
    }
    else if(opcode == 0xffd0)
    {
      // 1111111111010000 - go
      for(uint32_t i=reg_delay; i>0 && lightprogUploadSize==0; i--)
      {
        usbPoll();
        uint8_t v = 255 - i*255/reg_delay;
        setColor(blend(cur_col[0], reg_col[0], v), blend(cur_col[1], reg_col[1], v), blend(cur_col[2], reg_col[2], v));
        _delay_ms(1);
      }
      cur_col[0] = reg_col[0];
      cur_col[1] = reg_col[1];
      cur_col[2] = reg_col[2];
    }
    else if((opcode&0xffc0) == 0xe100)
    {
      // 1110000100 aaaaaa - jump
      reg_pc = (opcode&0x003f);
    }
  }
}



void calibrateOscillatorTest()
{
  calibrateOscillator();
  //D1("usb cal", OSCCAL);
  //eeprom_write_byte(0, OSCCAL);
}

int main()
{
  setupTimers();
  setColor(255, 0, 0);

  uint8_t calibrationValue = eeprom_read_byte(0);
  if(calibrationValue != 0xff)
  {
    setColor(0, 255, 0);
    OSCCAL = calibrationValue;
  }
  else
  {
    setColor(255, 255, 0);
    calibrateOscillator();
    eeprom_write_byte(0, OSCCAL);
  }
  _delay_ms(250);
  setColor(255, 0, 0);

  usbInit();
  usbDeviceDisconnect();
  _delay_ms(250);
  usbDeviceConnect();
  setColor(0, 0, 255);

  runLightVM();
  // for(;;)
  // {
  //   usbPoll();
  //   _delay_ms(1);
  // }
}

// Pin allocation:
// pin name usage                                   spi  used for
// 1   PB5  PCINT5/RESET/ADC0/dW                    +    debug wire (programming)
// 2   PB3  PCINT3/XTAL1/CLKI/OC1B/ADC3                   yellow led, serial out
// 3   PB4  PCINT4/XTAL2/CLKO/OC1B/ADC2
// 4   GND                                          +     ground
// 5   PB0  MOSI/DI/SDA/AIN0/OC0A/OC1A/AREF/PCINT0  +     USB D-
// 6   PB1  MISO/DO/AIN1/OC0B/OC1A/PCINT1           +     white led, dallas?
// 7   PB2  SCK/USCK/SCL/ADC1/T0/INT0/PCINT2        +     USB D+
// 8   VCC                                          +     power

