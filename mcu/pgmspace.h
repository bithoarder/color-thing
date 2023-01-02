#ifndef PGMSPACE_H
#define PGMSPACE_H

#include <avr/pgmspace.h>

struct pstr { char c; };

#undef PROGMEM
#define PROGMEM __attribute__(( section(".progmem.data") ))

#undef PSTR
#define PSTR(_STR) (__extension__({static char __c[] PROGMEM = (_STR); (const pstr*)&__c[0];}))

#endif

