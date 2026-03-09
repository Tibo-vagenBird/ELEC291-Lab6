#include <EFM8LB1.h>
#include "bootloader.h"
#include "config.h"

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN |= 0x80;       // enable VDD monitor
	RSTSRC = 0x02;  // Enable reset on missing clock detector and VDD

	#if (SYSCLK == 48000000L)	
		SFRPAGE = 0x10;
		PFE0CN  = 0x10; // SYSCLK < 50 MHz.
		SFRPAGE = 0x00;
	#elif (SYSCLK == 72000000L)
		SFRPAGE = 0x10;
		PFE0CN  = 0x20; // SYSCLK < 75 MHz.
		SFRPAGE = 0x00;
	#endif
	
	#if (SYSCLK == 12250000L)
		CLKSEL = 0x10;
		CLKSEL = 0x10;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 24500000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 48000000L)	
		// Before setting clock to 48 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x07;
		CLKSEL = 0x07;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 72000000L)
		// Before setting clock to 72 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x03;
		CLKSEL = 0x03;
		while ((CLKSEL & 0x80) == 0);
	#else
		#error SYSCLK must be either 12250000L, 24500000L, 48000000L, or 72000000L
	#endif

	SFRPAGE = 0x00;

	// ---- Port output mode: push-pull pins ----
	// P0.2 = UART0 TX, P0.4 = UART1 TX, P2.1 = IR carrier output
	P0MDOUT |= 0x14;           // P0.2 and P0.4 push-pull (UART TX lines)
	P2MDOUT |= 0b_0000_0010;   // P2.1 push-pull (IR 38 kHz output)

	// ---- Port digital input mode ----
	P2MDIN  |= 0b_0000_0010;   // P2.1 digital (not analog)

	// ---- Crossbar skip: pins used as GPIO must be skipped ----
	// P0SKIP: skip P0.0, P0.1, P0.6, P0.7 (unavailable/unused)
	// P2SKIP: skip P2.1 so crossbar never routes a peripheral to it
	P0SKIP = 0xC3;
	P2SKIP |= 0b_0000_0010;

	// ---- Enable crossbar AFTER all skip/mode registers are set ----
	// UART0 -> P0.2(TX), P0.3(RX)
	// UART1 -> P0.4(TX), P0.5(RX)
	XBR0 = 0x01;               // UART0 enable
	XBR1 = 0x00;
	XBR2 = 0x41;               // XBARE (bit6) + UART1EN (bit0)

	SFRPAGE = 0x00;

	return 0;
}

// Initialize P2.1 as the IR output pin (push-pull digital output, driven low).
// P2MDOUT, P2MDIN, and P2SKIP are already set in _c51_external_startup before
// the crossbar was enabled, so only the initial pin state is set here.
void init_pin_input(void){
	SFRPAGE = 0x00;
	P2_1 = 0;   // ensure IR output starts LOW (LED off)
}

