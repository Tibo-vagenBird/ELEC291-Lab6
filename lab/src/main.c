#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <string.h>
#include "usart.h"
#include "lcd.h"

/* Pinout for DIP28 ATMega328P:

                           -------
     (PCINT14/RESET) PC6 -|1    28|- PC5 (ADC5/SCL/PCINT13)
       (PCINT16/RXD) PD0 -|2    27|- PC4 (ADC4/SDA/PCINT12)
       (PCINT17/TXD) PD1 -|3    26|- PC3 (ADC3/PCINT11)
      (PCINT18/INT0) PD2 -|4    25|- PC2 (ADC2/PCINT10)
 (PCINT19/OC2B/INT1) PD3 -|5    24|- PC1 (ADC1/PCINT9)
    (PCINT20/XCK/T0) PD4 -|6    23|- PC0 (ADC0/PCINT8)
                     VCC -|7    22|- GND
                     GND -|8    21|- AREF
(PCINT6/XTAL1/TOSC1) PB6 -|9    20|- AVCC
(PCINT7/XTAL2/TOSC2) PB7 -|10   19|- PB5 (SCK/PCINT5)
   (PCINT21/OC0B/T1) PD5 -|11   18|- PB4 (MISO/PCINT4)
 (PCINT22/OC0A/AIN0) PD6 -|12   17|- PB3 (MOSI/OC2A/PCINT3)
      (PCINT23/AIN1) PD7 -|13   16|- PB2 (SS/OC1B/PCINT2)
  (PCINT0/CLKO/ICP1) PB0 -|14   15|- PB1 (OC1A/PCINT1)
                           -------
*/

// These are the connections between the LCD and the ATMega328P:
//
// LCD          ATMega328P
//----------------------------
// D7           PB0
// D6           PD7
// D5           PD6
// D4           PB1
// LCD_E        PD4
// LCD_W        GND
// LCD_RS       PD3
// V0           2k+GND
// VCC          5V
// GND          GND
// PD5 frequency in
// PB2 IR receiver out pin
// There is also a picture that shows how the LCD is attached to the ATMega328P.

// value of RA and RB
#define RA 			9841
#define RB			9811
#define CA			0.9775F
#define LA			0.269625737298F  // adjust to your actual value

volatile uint16_t t1_overflow_count = 0;

ISR(TIMER1_OVF_vect)
{
    t1_overflow_count++;
}

void Configure_Pins(void)
{
	DDRB|=0b00000011; // PB0, PB1 is output.
	DDRD|=0b11011000; // PD3, PD4, PD6, and PD7 are outputs.
}

void cal_resistence (unsigned long F, float* resistance) {
	*resistance = (1440000.0)/(CA * F) - 2 * RB;
}

// inductance calculation
void cal_inductance (unsigned long F, float* inductance) {
	*inductance = 1/(LA*F/1000); // inductance measured in H
	*inductance = (*inductance) * 1000; // inductance in mH
}

// capacitance calculation
// scaled by 1e6
void cal_capacitance (unsigned long F, float* capacitance) {
	*capacitance = 1440000.0/((RA+2*RB)*F);
	*capacitance = (*capacitance) * 1000;
}

// Reads PB2 using Timer0 (prescaler 64 -> 4 us/tick at 16 MHz).
// Returns:
//   0  if the measured period is ~552 us (138 ticks +/- 15)
//   1  if PB2 is constant HIGH (no LOW within ~1 ms)
// Prints measured ticks over UART for debugging.
uint8_t read_pb2_bit(void)
{
    uint8_t ticks;
    char dbg[32];

    // PB2 as input, no pull-up (IR receiver drives the line)
    DDRB  &= ~(1 << DDB2);
    PORTB &= ~(1 << PORTB2);

    // Timer0: normal mode, prescaler 64 -> 4 us per tick
    TCCR0A = 0x00;
    TCCR0B = (1 << CS01) | (1 << CS00);
    TCNT0  = 0;

    // Wait for a LOW edge; timeout at 250 ticks (~1 ms) -> constant HIGH
    while (PINB & (1 << PINB2)) {
        if (TCNT0 >= 250) {
            TCCR0B = 0x00;
            writeString("PB2: always HIGH\n");
            return 1;
        }
    }

    // Synchronise: wait for the next rising edge before timing
    while (!(PINB & (1 << PINB2)));
    TCNT0 = 0; // restart count at the rising edge

    // Measure HIGH half (rising -> falling)
    while (PINB & (1 << PINB2)) {
        if (TCNT0 >= 200) { TCCR0B = 0x00; return 1; }
    }

    // Measure LOW half (falling -> rising)
    while (!(PINB & (1 << PINB2))) {
        if (TCNT0 >= 200) { TCCR0B = 0x00; return 1; }
    }

    ticks = TCNT0; // full period in 4 us ticks
    TCCR0B = 0x00; // stop Timer0

    // Debug: print measured period
    sprintf(dbg, "PB2 ticks: %u (%u us)\n", ticks, (uint16_t)ticks * 4);
    writeString(dbg);

    // 552 us / 4 us = 138 ticks; accept +/- 15 ticks (60 us)
    if (ticks >= 123 && ticks <= 153) return 0;

    return 1;
}

static uint32_t count_t1_rising_edges_1s(void)
{
    uint16_t ovf;
    uint16_t low;
    uint8_t sreg;
    uint32_t total;

    // T1 pin = PD5 on ATmega328P family
    DDRD  &= ~(1 << DDD5);    // PD5 input
    PORTD &= ~(1 << PORTD5);  // no pull-up

    // Stop Timer1
    TCCR1A = 0x00;
    TCCR1B = 0x00;

    // Normal mode
    TCCR1A = 0x00;

    // Clear software overflow counter
    t1_overflow_count = 0;

    // Clear hardware counter
    TCNT1 = 0;

    // Clear pending overflow flag
    TIFR1 = (1 << TOV1);

    // Enable Timer1 overflow interrupt
    TIMSK1 = (1 << TOIE1);

    // Enable global interrupts
    sei();

    // Start Timer1:
    // CS12:0 = 111 => external clock on T1 pin, rising edge
    TCCR1B = (1 << CS12) | (1 << CS11) | (1 << CS10);

    // Gate time = about 1 second
    _delay_ms(1000);

    // Stop Timer1 first so count no longer changes
    TCCR1B = 0x00;

    // Read 32-bit result atomically
    sreg = SREG;
    cli();
    ovf = t1_overflow_count;
    low = TCNT1;

    // If overflow happened but ISR has not run yet, fix it
    if (TIFR1 & (1 << TOV1))
    {
        ovf++;
        low = TCNT1;
    }
    SREG = sreg;

    // Disable overflow interrupt if no longer needed
    TIMSK1 = 0x00;

    total = ((uint32_t)ovf << 16) | low;
    return total;
}

int main( void )
{
	char buff[17];
	char uart_buff[32];
	char ir_buf[32];
	uint32_t pulse_count;
	uint8_t ir_addr, ir_cmd;
	int8_t  nec_result;
    float capacitance;
    float resistance;
    uint8_t c_or_r;

    Configure_Pins();
	LCD_4BIT();
	initUART(); // configure the usart and baudrate

	_delay_ms(500); // Give putty some time to start.
	printf("ATMega328P 4-bit LCD test.\n");

   	// Display something in the LCD
	LCDprint("LCD 4-bit test:", 1, 1);
	LCDprint("Hello, World!", 2, 1);
	while(1)
	{
		pulse_count = count_t1_rising_edges_1s();
        cal_capacitance(pulse_count, &capacitance);
        cal_resistence(pulse_count, &resistance);

        c_or_r = read_pb2_bit();

		sprintf(uart_buff, "freq: %lu %f c_or_r: %u\n", pulse_count, capacitance, c_or_r);
		writeString(uart_buff);
        if (c_or_r == 0) {
            sprintf(buff, "%.2f", capacitance);
            LCDprint("Capacitance:", 1, 1);
        }
        else {
            sprintf(buff, "%.2f", resistance);
            LCDprint("Resistance:", 1, 1);
        }
        LCDprint(buff, 2, 1);

        _delay_ms(500);
	}
}
