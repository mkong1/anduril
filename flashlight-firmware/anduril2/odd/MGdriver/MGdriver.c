/*
 *
 * MGdriver -  Off-time MultiGroup Driver by odd, based on luxdrv030 by DrJones
 *
 *
 * License: CC-BY-NC-SA (non-commercial use only, derivative works must be under the same license)
 *
 *
 *  Main differences to luxdrvß30:
 *  - REMOVED: Ramping mode
 *  - REMOVED: Wear leveling
 *  - CHANGED: OFFTime memory instead of ONTime memory
 *  - ADDED: Multiple groups
 *  - ADDED: Memory or nomemory selectable for each group
 *  - ADDED: Simple UI for switching groups
 */


#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay_basic.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#define F_CPU 			4800000
#define byte 			uint8_t
#define word 			uint16_t
#define NOMEM 			0
#define MEM 			1
#define NOINIT			__attribute__ ((section (".noinit")))
#define STROBE  		253
#define BEACON			252
#define MODES_0			NOMEM,	255, 0 , 0, 0 , 0			// Fill unused modes with pwm level 0
#define MODES_1			MEM,	25,	255 , 0 , 0 , 0
#define MODES_2			NOMEM,	25,	255 , 0 , 0 , 0
#define MODES_3			MEM, 	255, 25 , 0 , 0 , 0
#define MODES_4			NOMEM, 	255, 25 , 0 , 0 , 0
#define MODES_5			MEM,	25, 64 , 255 , 0 , 0
#define MODES_6			NOMEM,	25, 64 , 255 , 0 , 0
#define MODES_7			MEM,	255, 64 , 25 , 0 , 0
#define MODES_8			NOMEM,	255, 64 , 25 , 0 , 0
#define MODES_9			NOMEM,	6, 25 , 64 , 255 , 0
#define MODES_10		NOMEM,	25, 64 , 255 , STROBE , 0
#define MODES_11		NOMEM,	6 , 25 , 64 ,BEACON, STROBE
#define LENGTH_MODES	6									// 5 modes per group +1 for memory seletion
#define NUM_GROUPS		12									// 12 groups
PROGMEM const byte groups[NUM_GROUPS][LENGTH_MODES]={	
							{MODES_0} , {MODES_1} ,	//delete or add groups here
							{MODES_2} , {MODES_3} ,
							{MODES_4} , {MODES_5} ,
							{MODES_6} , {MODES_7} ,
							{MODES_8} , {MODES_9} ,
							{MODES_10}, {MODES_11} };

#define RESETTIME 12
#define BATTMON  129

#define outpin 1
#define PWM OCR0B
#define adcpin 2
#define adcchn 1
#define portinit() do{ DDRB=(1<<outpin); PORTB=0xff-(1<<outpin)-(1<<adcpin);  }while(0)

#define WDTIME 0b01000011  //125ms

#define sleepinit() do{ WDTCR=WDTIME; sei(); MCUCR=(MCUCR &~0b00111000)|0b00100000; }while(0)

#define SLEEP asm volatile ("SLEEP")

#define pwminit() do{ TCCR0A=0b00100001; TCCR0B=0b00000001; }while(0)

#define adcinit() do{ ADMUX =0b01100000|adcchn; ADCSRA=0b11000100; }while(0)
#define adcread() do{ ADCSRA|=64; while (ADCSRA&64); }while(0)
#define adcresult ADCH

#define ADCoff ADCSRA&=~(1<<7)
#define ADCon  ADCSRA|=(1<<7)
#define ACoff  ACSR|=(1<<7)
#define ACon   ACSR&=~(1<<7)


EEMEM byte eemode = 1;				//Saving mode and group in EEPROM, no wear leveling
EEMEM byte eegroup = 1;
byte mypwm = 0;
volatile byte ticks = 0;
byte mode = 1;
byte pmode = 50;
byte group= 0 ;
byte memory = 0;
byte i = 0;
byte decay NOINIT;					//Using the NOINIT section of the RAM
byte shortcounter NOINIT;
byte lowbattcounter = 0;

ISR(WDT_vect) {
	if (ticks < 255)
		ticks++;
	if (ticks == RESETTIME)
		shortcounter=0;  			// Setting back the counter for repeated shortpresses, so we
									// never enter programming mode by accident


#ifdef BATTMON
	adcread()
	;
	if (adcresult < BATTMON) {
		if (++lowbattcounter > 40) {
			mypwm = (mypwm >> 1) + 3;
			lowbattcounter = 0;
		}
	} else
		lowbattcounter = 0;
#endif
}


void _delay_ms(uint16_t n) {

    while(n-- > 0) _delay_loop_2(950);
}


inline void getmode(void) {

	mode=eeprom_read_byte(&eemode);						//reading mode and group from EEPROM
	group=eeprom_read_byte(&eegroup);
	memory=pgm_read_byte(&groups[group][0]);			// reading memory from the 2D-array stored in flash
	if (!decay) {										// fast press
		mode++;
									//wrap around if it is last mode of the group or a unused mode
	    if ((mode > LENGTH_MODES) || (!(pgm_read_byte(&groups[group][mode]))))
	    	mode = 1;

	    	shortcounter += 1;							//count number of short-presses
		}
		else {											// long press
			if (memory == NOMEM) {
	  		  mode=1;  //NOMEM

		}
	  	shortcounter = 0;								//Set back the counter for repeated shortpresses
		}
	  decay=0;											//Set back decay for long/shortpress detection

	  eeprom_write_byte(&eemode, mode);
}


int main(void) {


	portinit();

	sleepinit();

	ACoff;

#ifdef BATTMON
	adcinit();
#else
	ADCoff;
#endif

	pwminit();

	getmode();

	pmode= pgm_read_byte(&groups[group][mode]);

	if (shortcounter>15) {								// 15 or more short presses, enter programming mode

		PWM = 25;
		_delay_ms(2000);								// to switch to the next group, press switch when light
		shortcounter = 0;								// goes off after 2sec, or wait until it turn on again to cancel
														// switching the group
		group++;
		if (group >= NUM_GROUPS)
			group = 0;
		shortcounter = 0;
		eeprom_write_byte(&eegroup, group);
		eeprom_write_byte(&eemode, 1);
		PWM = 0;
		_delay_ms(2000);
		if (!group)
			group--;
		else
			group = (NUM_GROUPS-1);
		eeprom_write_byte(&eegroup, group);
		while (1) {
			PWM = 25;
		}

		}

	else {

			switch (pmode) {

		#ifdef STROBE
			case STROBE:
				mypwm = 255;
				while (1) {
					PWM = mypwm;
					_delay_ms(20);
					PWM = 0;
					_delay_ms(60);
				}
				break; //strobe 12.5Hz  //+48
		#endif

		#ifdef BEACON
			case BEACON:
		#ifdef BATTMON

				adcread();
				i = adcresult;
				while (i > BATTMON) {
					PWM = 8;
					SLEEP;					//SLEEP instead of delay_ms saves a few bytes here
					SLEEP;
					SLEEP;
					PWM = 0;
					SLEEP;
					SLEEP;
					i -= 5;
				}
				SLEEP;
				SLEEP;
				SLEEP;
		#endif
				mypwm = 255;
				while (1) {
					PWM = mypwm;
					_delay_ms(20);
					PWM = 0;
					_delay_ms(9880);
				}
				break;  //beacon 10s //+48
		#endif

			default:
				mypwm = pmode;
				while (1) {
					PWM = mypwm;
					SLEEP;
				}
			}
}
		return 0;
}