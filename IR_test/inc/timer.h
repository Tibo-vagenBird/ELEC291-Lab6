#ifndef TIMER_H
#define TIMER_H

#define TIMER0_RELOAD 64588
#define TIMER2_RELOAD 46588

// Timer3 reload for 1 us overflow at SYSCLK = 72 MHz
// Each overflow = 1 us; used by Timer3us/waitms for NEC timing
#define TIMER3_1US_RELOAD (65536 - (SYSCLK / 1000000L))  /* 65464 at 72 MHz */

void TIMER0_Init(void);
void Timer3_Init(void);
void Timer3us(unsigned int us);
void waitms(unsigned int ms);

#endif
