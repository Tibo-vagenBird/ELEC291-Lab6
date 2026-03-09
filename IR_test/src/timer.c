#include <EFM8LB1.h>
#include "timer.h"

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
	TR2 = 0;
	CKCON0 |= 0b_0011_0000; // set Timer 2 to be using sysclk
	T2SPLIT = 0;
	TMR2RLH = (TIMER2_RELOAD >> 8) &0xFF;
	TMR2RLL = TIMER2_RELOAD & 0xFF;
	TMR2H = (TIMER2_RELOAD >> 8) &0xFF;
	TMR2L = TIMER2_RELOAD & 0xFF;

	// enable interrupt
	ET2 = 1;
	EA = 1;
	TR2 = 1; // start timer 2
}

void Timer2_ISR(void) __interrupt (5)
{
	TR2 = 0; // stop timer 2
	ET2 = 0; // disable interrupt
	TF2H = 0;

	P2_2 = !P2_2; // toggle p2.2 for oscilloscope debugging purpose
	// toggle timer 0
	if (TR0)
    {
        TR0 = 0;
        ET0 = 0;
        P2_1 = 0;
    }
    else
    {
        TH0 = (TIMER0_RELOAD >> 8) & 0xFF;
        TL0 = TIMER0_RELOAD & 0xFF;
        TF0 = 0;
        ET0 = 1;
        TR0 = 1;
    }

	ET2 = 1;
	TR2 = 1;
	
}


// Uses Timer3 to delay <us> micro-seconds. 
void Timer3us(unsigned char us)
{
	unsigned char i;               // usec counter
	
	// The input for Timer 3 is selected as SYSCLK by setting T3ML (bit 6) of CKCON0:
	CKCON0|=0b_0100_0000;
	
	TMR3RL = (-(SYSCLK)/1000000L); // Set Timer3 to overflow in 1us.
	TMR3 = TMR3RL;                 // Initialize Timer3 for first overflow
	
	TMR3CN0 = 0x04;                 // Sart Timer3 and clear overflow flag
	for (i = 0; i < us; i++)       // Count <us> overflows
	{
		while (!(TMR3CN0 & 0x80));  // Wait for overflow
		TMR3CN0 &= ~(0x80);         // Clear overflow indicator
	}
	TMR3CN0 = 0 ;                   // Stop Timer3 and clear overflow flag
}

void waitms (unsigned int ms)
{
	unsigned int j;
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<4; k++) Timer3us(250);
}

