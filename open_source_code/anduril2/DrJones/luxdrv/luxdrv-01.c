//
// LuxDrv 0.1    DrJones 2011 
//
// 630 bytes (with timer)
//
// License: Free for private and noncommercial use for members of BudgetLightForum


//ToDo: 
//  Batt-ADC (ADC1) with mode down-shifting 
//  BATT indicator using blinks in beacon-mode
//  ? Ramping
//  ? off-time checking; requires adding  diode,R,C
//  ? blink your name in morse code :)




#define outpin 1
 #define PWM OCR0B  //PWM-value

#define portinit() do{ DDRB=(1<<outpin); PORTB=0xff-(1<<outpin);  }while(0)



#include <avr/pgmspace.h>
#define byte uint8_t
#define word uint16_t


//########################################################################################### MODE/PWM SETUP 

//251, 252, 253 are special mode codes handled differently, all other are PWM values

#define F_CPU 4800000    //CPU: 4.8MHz  PWM: 9.4kHz    // low fuse: 0x75    //+16bytes compared to 1.2MHz 
PROGMEM byte modes[]={ 255,    6,51,255,    5,6,14,37,98,255,   251,   252,   253  };  // 9kHz modes, 5 is lowest
//                     dummy   main modes   more levels         strobe,beacon,timer


//#define F_CPU 1200000  //CPU: 1.2MHz  PWM: 2.35kHz  //low fuse: 0x66
//PROGMEM byte modes[]={ 255,    2,51,255,    2,5,14,37,98,255,   251,   252,   253  };  //2.35kHz modes, 2 is lowest
////                     dummy   main modes   more levels         strobe,beacon,timer


//#define F_CPU 600000   //CPU: 0.6MHz  PWM: 1.18kHz   //low fuse: 0x65
//PROGMEM byte modes[]={ 255,    2,51,255,    1,5,14,37,98,255,   251,   252,   253  };  //1.18kHz modes, 1 is lowest
////                     dummy   main modes   more levels         strobe,beacon,timer


//###########################################################################################

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>


#define WDTIME 0b01000011  //125ms

#define sleepinit() do{ WDTCR=WDTIME; sei(); MCUCR=(MCUCR &~0b00111000)|0b00100000; }while(0) //WDT-int and Idle-Sleep

#define pwminit() do{ TCCR0A=0b00100001; TCCR0B=0b00000001; }while(0)  //chan A, phasePWM, clk/1  ->2.35kHz@1.2MHz


#define SLEEP asm volatile ("SLEEP")
#define ADCoff ADCSRA&=~(1<<7) //ADC off (enable=0);
#define ADCon  ADCSRA|=(1<<7)  //ADC on
#define ACoff  ACSR|=(1<<7)    //AC off (disable=1)
#define ACon   ACSR&=~(1<<7)   //AC on  (disable=0)


//_____________________________________________________________________________________________________________________


//saving a few byte by doing that inline
inline void eepwrite(byte addr, byte data) {  while(EECR & 2); EEARL=addr; EEDR=data; EECR =4; EECR =6;  }
inline byte eepread(byte addr)             {  while(EECR & 2); EEARL=addr; EECR=1; return EEDR;  }


volatile byte mypwm=0;
volatile byte ticks=0;
byte mode=1;
byte pmode=50;

byte eep[32];  //EEPROM buffer
byte eepos=0;


inline void getmode(void) {  //read current mode from EEPROM and write next mode

  eeprom_read_block(&eep, 0, sizeof(eep));  //+44                //read block 
  while((eep[eepos]==0) && (eepos<sizeof(eep))) eepos++; //+16   //find mode byte
  if (eepos<sizeof(eep)) mode=eep[eepos];
  else eepos=0;   //+6  //not found

  byte next=1;
  if (mode & 0x80) {  //last on-time was short
    mode&=0x7f;   if (mode>=sizeof(modes)) mode=1;
    next=mode+1;  if (next>=sizeof(modes)) next=1; 
  } //else next=1;  //previous mode was locked, this one is yet a short on, so restart from 1st mode.
  eepwrite(eepos,next|0x80); //write next mode, with short-on marker
}

inline void savemode(void) {   //lock mode: keep this mode for next time
  eepwrite(eepos,0);  
  eepos=(eepos+1)&31;   //+12  //wear leveling, use next cell
  eepwrite(eepos,mode);
}




ISR(WDT_vect) {   //WatchDogTimer Interrupt
if (ticks<255) ticks++;
if (ticks==16) savemode();  //lock mode after 2s
}




int main(void) {

  portinit();
  sleepinit();
  ACoff;
  ADCoff;
  pwminit();

  getmode();  //get mode# to use

  pmode=pgm_read_byte(&modes[mode]);  //read actual PWM value/special code
  byte i;

  switch(pmode){

    case 251:  mypwm=255; while(1){ PWM=mypwm; _delay_ms(20); PWM=0; _delay_ms(60); } break;                 //strobe 12.5Hz  //+48

    case 252:  mypwm=255; while(1){ PWM=mypwm; _delay_ms(20); PWM=0; i=70;do{SLEEP;}while(--i);  } break;  //beacon 10s     //+48

    case 253:  i=5; do{ byte j=i; do{PWM=255; _delay_ms(20); PWM=8; _delay_ms(300); }while(--j); //blink remaining minutes   
                             byte k=59;do{_delay_ms(1000);PWM^=8;}while(--k);                      //wait 1 minute
                         }while(--i);                                                               //for 5 min
               i=100; do{ PWM=255; _delay_ms(30); PWM=0; _delay_ms(70); }while(--i);              //strobe 10s
               while(1){PWM^=8;SLEEP;}                                                              //"off"                 //+136
               break;

    default:   mypwm=pmode; while(1){PWM=mypwm;SLEEP;}   //all other: PWM value

   //mypwm: prepared for being ramped down in WDT on low-batt condition.

  }//switch
  return 0;
}//main

