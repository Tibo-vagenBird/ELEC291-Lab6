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
// D4           PD5
// LCD_E        PD4
// LCD_W        GND
// LCD_RS       PD3
// V0           2k+GND
// VCC          5V
// GND          GND
//
// There is also a picture that shows how the LCD is attached to the ATMega328P.

// value of RA and RB
#define RA 			9841
#define RB			9811
#define CA			1.11F
#define LA			0.269625737298F  // adjust to your actual value

void Configure_Pins(void)
{
	DDRB|=0b00000001; // PB0 is output.
	DDRD|=0b11011000; // PD3, PD4, PD5, PD6, and PD7 are outputs.
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

volatile uint16_t t1_overflow_count = 0;

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
	uint32_t pulse_count;

	initUART(); // configure the usart and baudrate
	
	_delay_ms(500); // Give putty some time to start.
	printf("ATMega328P 4-bit LCD test.\n");

   	// Display something in the LCD
	LCDprint("LCD 4-bit test:", 1, 1);
	LCDprint("Hello, World!", 2, 1);
	while(1)
	{
		pulse_count = count_t1_rising_edges_1s();
		sprintf(buff, "freq: %u\n", pulse_count);
		writeString(buff);

        // pulse_count now holds:
        // number of rising edges on T1 during ~1 second

        // Put breakpoint here, print by UART, or show on LEDs/LCD
        _delay_ms(500);
	}
}
