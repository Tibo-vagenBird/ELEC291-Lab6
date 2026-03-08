//  LCD in 4-bit interface mode
#include <avr/io.h>
#include "lcd.h"
#include <util/delay.h>

void LCD_pulse (void)
{
	LCD_E_1;
	_delay_us(40);
	LCD_E_0;
}

void LCD_byte (unsigned char x)
{
	//Send high nible
	if(x&0x80) LCD_D7_1; else LCD_D7_0;
	if(x&0x40) LCD_D6_1; else LCD_D6_0;
	if(x&0x20) LCD_D5_1; else LCD_D5_0;
	if(x&0x10) LCD_D4_1; else LCD_D4_0;
	LCD_pulse();
	_delay_us(40);
	//Send low nible
	if(x&0x08) LCD_D7_1; else LCD_D7_0;
	if(x&0x04) LCD_D6_1; else LCD_D6_0;
	if(x&0x02) LCD_D5_1; else LCD_D5_0;
	if(x&0x01) LCD_D4_1; else LCD_D4_0;
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS_1;
	LCD_byte(x);
	_delay_ms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS_0;
	LCD_byte(x);
	_delay_ms(5);
}

void LCD_4BIT (void)
{
	LCD_E_0; // Resting state of LCD's enable is zero
	LCD_RS_0;
	_delay_ms(50); // HD44780 requires >40ms after power-on

	// Reset sequence: send nibble 0x3 three times individually (one E pulse each)
	// with mandatory delays between them (>4.1ms, >100us, >100us)
	LCD_D7_0; LCD_D6_0; LCD_D5_1; LCD_D4_1; // nibble = 0x3
	LCD_pulse(); _delay_ms(5);

	LCD_D7_0; LCD_D6_0; LCD_D5_1; LCD_D4_1; // nibble = 0x3
	LCD_pulse(); _delay_ms(1);

	LCD_D7_0; LCD_D6_0; LCD_D5_1; LCD_D4_1; // nibble = 0x3
	LCD_pulse(); _delay_ms(1);

	// Switch to 4-bit mode: send nibble 0x2
	LCD_D7_0; LCD_D6_0; LCD_D5_1; LCD_D4_0; // nibble = 0x2
	LCD_pulse(); _delay_ms(1);

	// Now in 4-bit mode — configure the LCD
	WriteCommand(0x28); // Function set: 4-bit, 2-line, 5x8
	WriteCommand(0x08); // Display off
	WriteCommand(0x01); // Clear display
	_delay_ms(2);       // Clear needs >1.52ms
	WriteCommand(0x06); // Entry mode: increment cursor, no shift
	WriteCommand(0x0C); // Display on, cursor off, blink off
}

void LCDprint(char * string, unsigned char line, unsigned char clear)
{
	int j;

	WriteCommand(line==2?0xc0:0x80);
	_delay_ms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}
