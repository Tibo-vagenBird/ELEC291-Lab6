#include <EFM8LB1.h>
#include "timer.h"
#include "config.h"

volatile unsigned long timer0_isr_ticks = 0;

void TIMER0_Init(void)
{
	TR0 = 0; // Stop Timer 0 during setup
	TMOD &= 0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD |= 0b_0000_0001; // Timer/Counter 0 used as a 16-bit timer
	CKCON0 &= 0b_1111_1000; // Reset timer 0 setup
	CKCON0 |= 0b_0000_0100;  // Timer 0 uses sysclk
	
	TH0 = (TIMER0_RELOAD >> 8) & 0xFF;
	TL0 = TIMER0_RELOAD & 0xFF;
	TF0 = 0; // Clear pending overflow flag

	// Enable interrupt
	ET0 = 1;
	EA  = 1;
	TR0 = 1; // Start Timer 0
}

void Timer0_ISR(void) __interrupt (1)
{
	// Mode 1 is 16-bit, so reload on every overflow.
	TR0 = 0;
	TH0 = (TIMER0_RELOAD >> 8) & 0xFF;
	TL0 = TIMER0_RELOAD & 0xFF;
	P2_1 = !P2_1;
	TR0 = 1;
}

void TIMER2_Init(void){
	CKCON0 |= 0b_0011_0000; // set Timer 2 to be using sysclk
	TMR2CN0 = 0x00;
	TMR2RLH = (TIMER2_RELOAD >> 8) &0xFF;
	TMR2RLL = TIMER2_RELOAD & 0xFF;

	// enable interrupt
	ET2 = 1;
	EA = 1;
	TR2 = 1; // start timer 2
}

void Timer2_ISR(void) __interrupt (5)
{
	TR2 = 0; // stop timer 2
	ET2 = 0; // disable interrupt

	// toggle timer 0
	ET0 = !ET0;
	TR0 = !TR0;
}

// Configure Timer3 to overflow once per microsecond at SYSCLK = 72 MHz.
// Must be called before Timer3us() or waitms().
void Timer3_Init(void)
{
	CKCON0 |= 0b_0100_0000;                        // T3ML=1: Timer3 clock = SYSCLK
	TMR3RLH = (TIMER3_1US_RELOAD >> 8) & 0xFF;    // reload high byte
	TMR3RLL =  TIMER3_1US_RELOAD       & 0xFF;    // reload low byte
	TMR3H   = (TIMER3_1US_RELOAD >> 8) & 0xFF;    // preload counter
	TMR3L   =  TIMER3_1US_RELOAD       & 0xFF;
	TMR3CN0 = 0x00;                                // stop, clear flags
}

// Delay approximately <us> microseconds using Timer3.
// Timer3_Init() must be called once before using this function.
void Timer3us(unsigned int us)
{
	unsigned int i;
	TMR3CN0 = 0x04;               // TR3=1: start Timer3
	for (i = 0; i < us; i++)
	{
		while (!(TMR3CN0 & 0x80)); // wait for TF3H (overflow)
		TMR3CN0 &= ~0x80;          // clear overflow flag
	}
	TMR3CN0 = 0x00;               // stop Timer3
}

// Delay approximately <ms> milliseconds.
void waitms(unsigned int ms)
{
	unsigned int i;
	for (i = 0; i < ms; i++)
		Timer3us(1000);
}
