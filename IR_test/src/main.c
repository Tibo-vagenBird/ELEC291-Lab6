#include <stdio.h>
#include <stdlib.h>
#include <EFM8LB1.h>
#include <string.h>
#include "bootloader.h"
#include "timer.h"
#include "config.h"

// ---- Global state variables ----





// ---- Main ----
void main (){
	init_pin_input();
	TIMER0_Init();
	TIMER2_Init();
	
	while(1){

	}
}
