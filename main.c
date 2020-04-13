/*
 *
 * Frequency inverter microcontroller code
 *
 * Author: Marco Antônio Habitzreuter (mahabitzreuter@gmail.com)
 * Date: oct 20 2015
 *
 */

/********************************************************************************
  Included files
 ********************************************************************************/
#include <htc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"
/********************************************************************************
  PIC configuration
 ********************************************************************************/
__CONFIG(FOSC_INTRCIO & WDTE_OFF & PWRTE_ON & MCLRE_ON);
#ifndef _XTAL_FREQ // Unless already defined assume 4MHz system frequency
#define _XTAL_FREQ 400000
#endif
/********************************************************************************
  Definitions
 ********************************************************************************/
#define BT_1 RA2
#define BT_1_EN IOCA2
#define BT_2 RA4
#define BT_2_EN IOCA4
#define BT_3 RA5
#define BT_3_EN IOCA5
/********************************************************************************
  Global variables
 ********************************************************************************/
bit flagCycle = 0;
char dutyCounter = 0;
char dutyCycle [19];
char frequency = 60;
int TMR1Val;
/********************************************************************************
  Function prototypes and description
 ********************************************************************************/
void interrupt interr();
int calcTMR1(char freq);
void calcSinArray();
void start();
void stop();
/********************************************************************************
  Main function
 ********************************************************************************/
void main() {
	ANSELH = ANSEL = 0;
	TRISA = 0b00110100;
	TRISB = 0;
	TRISC = 0b00111100;
	PORTA = PORTB = PORTC = 0;
	INTCON = 0b11000000;
	nRABPU = 0;
	//PWM, TMR1 and TMR2
	TMR1 = TMR1Val = calcTMR1(frequency);
	PR2 = 14;
	calcSinArray();
	CCP1CON = 0b01001100;
	T2CON = 0b00000110;
	TMR1IE = 1;
	TMR1IF = 0;
	//buttons
	BT_1_EN = BT_2_EN = BT_3_EN = 1;
	lcd_init();
	while (1) {
		if (!RABIE) {
			if (!BT_1_EN) {
				while (!BT_1) {}
				__delay_ms(20);
				BT_1_EN = 1;
				if (!TMR1ON) start();
				else stop();
			} else if (!BT_2_EN) {
				while (!BT_2) {}
				__delay_ms(40);
				BT_2_EN = 1;
				if (frequency < 250) frequency++;
			} else if (!BT_3_EN) {
				while (!BT_3) {}
				__delay_ms(20);
				BT_3_EN = 1;
				if (frequency > 15) frequency--;
			}
			TMR1 = TMR1Val = calcTMR1(frequency);
			lcd_clear();
			printf("    Inversor\nFrequencia: %dHz", frequency);
			RABIE = 1;
		}
	}
}

void interrupt interr(void) {
	if (TMR1IF) {
		TMR1IF = 0;
		TMR1 = TMR1Val;
		if (!flagCycle) {
			if (++dutyCounter == 18) flagCycle = 1;
		} else if (flagCycle) {
			//reverse/forward brigde
			if (--dutyCounter == 0) flagCycle = 0, P1M1 = !P1M1;
		}
		CCPR1L = dutyCycle [ dutyCounter ];
	} else if (T0IF) {
		T0IF = 0;
	} else if (RABIF) {
		if (!BT_1) BT_1_EN = 0;
		else if (!BT_2) BT_2_EN = 0;
		else if (!BT_3) BT_3_EN = 0;
		PORTB = PORTB;
		RABIF = RABIE = 0;
	}
}

void calcSinArray() {
	char x = 0;
	for (x = 0; x < 19; x++) {
		dutyCycle[x] = (sin((5 * x) * (M_PI / 180))) * (PR2 + 1);
	}
	dutyCycle[1] = 0;
}

int calcTMR1(char freq) {
	float timer = 65535 - (((1 / ((float) freq * 2.0)) / 38.0) * 1000000.0);
	return timer;
}

void start() {
	TMR1ON = 1;
	TRISC2 = TRISC3 = TRISC4 = TRISC5 = 0;
}

void stop() {
	TMR1ON = 0;
	TRISC2 = TRISC3 = TRISC4 = TRISC5 = 1;
}
