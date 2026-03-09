#include <stdio.h>
#include <stdlib.h>
#include <EFM8LB1.h>
#include <string.h>
#include "bootloader.h"
#include "timer.h"
#include "config.h"

// ---- NEC IR Transmitter on P2_1 ----
//
// Timer0 generates the 38 kHz carrier by toggling P2_1 in its ISR.
// The transmitter modulates the carrier by enabling/disabling Timer0.
//
// NEC frame timing:
//   Leader burst  : 9000 us carrier ON
//   Leader space  : 4500 us carrier OFF
//   Bit burst     :  562 us carrier ON   (same for 0 and 1)
//   Bit-0 space   :  563 us carrier OFF  -> total ~1.125 ms
//   Bit-1 space   : 1688 us carrier OFF  -> total ~2.25 ms
//   Stop burst    :  562 us carrier ON
//   Repeat space  : 2250 us carrier OFF  (used in repeat frames)
//
// Frame bit order: LSB first, 32 bits = [addr][~addr][cmd][~cmd]

// Start 38 kHz carrier: reload and start Timer0
static void ir_carrier_on(void)
{
	TF0 = 0;
	TH0 = (TIMER0_RELOAD >> 8) & 0xFF;
	TL0 =  TIMER0_RELOAD       & 0xFF;
	ET0 = 1;
	TR0 = 1;
}

// Stop carrier: halt Timer0 and drive P2_1 LOW (LED off)
static void ir_carrier_off(void)
{
	TR0 = 0;
	ET0 = 0;
	P2_1 = 0;
}

// Send 38 kHz carrier burst for <us> microseconds
static void ir_burst(unsigned int us)
{
	ir_carrier_on();
	Timer3us(us);
	ir_carrier_off();
}

// Send silence (carrier off) for <us> microseconds
static void ir_space(unsigned int us)
{
	ir_carrier_off();
	Timer3us(us);
}

// Send one NEC data bit (burst + space)
// bit_val = 0: 562 us burst + 563 us space
// bit_val = 1: 562 us burst + 1688 us space
static void nec_send_bit(unsigned char bit_val)
{
	ir_burst(562);
	if (bit_val)
		ir_space(1688);
	else
		ir_space(563);
}

// Send one byte LSB first (NEC convention)
static void nec_send_byte(unsigned char b)
{
	unsigned char i;
	for (i = 0; i < 8; i++)
	{
		nec_send_bit(b & 0x01);
		b >>= 1;
	}
}

// Transmit a full NEC frame for the given address and command.
// Both the byte and its logical inverse are sent automatically.
void nec_transmit(unsigned char addr, unsigned char cmd)
{
	// Leader code
	ir_burst(9000);
	ir_space(4500);

	// 32 data bits: addr, ~addr, cmd, ~cmd  (LSB first each byte)
	nec_send_byte(addr);
	nec_send_byte((unsigned char)(~addr));
	nec_send_byte(cmd);
	nec_send_byte((unsigned char)(~cmd));

	// Stop bit
	ir_burst(562);
	ir_space(563);
}

// Transmit an NEC repeat frame (send while key is held, ~108 ms after first frame).
void nec_repeat(void)
{
	ir_burst(9000);
	ir_space(2250);
	ir_burst(562);
	ir_space(563);
}

// ---- Main ----
void main(void)
{
	init_pin_input();
	TIMER0_Init();
	Timer3_Init();

	// Example: transmit NEC address=0x00, command=0x0C
	// then send three repeat frames to simulate a held key.
	nec_transmit(0x00, 0x0C);
	waitms(40);
	nec_repeat();
	waitms(40);
	nec_repeat();
	waitms(40);
	nec_repeat();

	while(1)
	{
		// Retransmit every 200 ms for continuous testing.
		nec_transmit(0x00, 0x0C);
		waitms(200);
	}
}