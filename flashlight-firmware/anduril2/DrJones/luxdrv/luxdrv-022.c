//
// LuxDrv 0.22    DrJones 2011 
//
// 878 bytes with ramping, 696 bytes without   (both with new timer)  
//
// License: Free for private and noncommercial use for members of BudgetLightForum
//
// changes:
// 0.1  first version: levels,strobe,beacon,timer(136)   (638bytes)
// 0.2  ramping(200), improved timer(180), skip mode1 as next mode if already in mode 1     (898bytes)
// 0.21 changed EEPROM handling to reduce power-off-while-writing glitches -->eepsave(byte)  (874bytes)
// 0.22 bugfix in ramping; mode not stored on (very) short tap   (878bytes)



#define outpin 1
 #define PWM OCR0B  //PWM-value

#define adcpin 2
 #define adcchn 1

#define portinit() do{ DDRB=(1<<outpin); PORTB=0xff-(1<<outpin)-(1<<adcpin);  }while(0)



#include <avr/pgmspace.h>
#define byte uint8_t
#define word uint16_t


//########################################################################################### MODE/PWM SETUP 

//251, 252, 253, 254 are special mode codes handled differently, all other are PWM values

#define F_CPU 4800000    //CPU: 4.8MHz  PWM: 9.4kHz       ####### use low fuse: 0x75  #######   
PROGMEM byte modes[]={ 0,      6,51,255,    254,                   251,    252,    253 };  // with ramping
//                     dummy   main modes   ramping, ramped mode,  strobe, beacon, timer
#define TIMER 5     //use timer (5 min, max 6 in this setup)
#define RAMPING     //use ramping
#define MINPWM   5  //needed for ramping
#define RAMPMODE 4  //the number of the ramping mode (254) in the modes[] order; e.g. ramping is 4th mode -> 4; not counting dummy


//PROGMEM byte modes[]={ 0,      6,51,255,    5,6,14,37,98,255,   251,   252,   253  };  // 9kHz modes, 5 is lowest, no ramping
////                     dummy   main modes   more levels         strobe,beacon,timer


//###########################################################################################

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>



#define WDTIME 0b01000011  //125ms

#define sleepinit() do{ WDTCR=WDTIME; sei(); MCUCR=(MCUCR &~0b00111000)|0b00100000; }while(0) //WDT-int and Idle-Sleep

#define SLEEP asm volatile ("SLEEP")

#define pwminit() do{ TCCR0A=0b00100001; TCCR0B=0b00000001; }while(0)  //chan A, phasePWM, clk/1  ->2.35kHz@1.2MHz



#define ADCoff ADCSRA&=~(1<<7) //ADC off (enable=0);
#define ADCon  ADCSRA|=(1<<7)  //ADC on
#define ACoff  ACSR|=(1<<7)    //AC off (disable=1)
#define ACon   ACSR&=~(1<<7)   //AC on  (disable=0)


//_____________________________________________________________________________________________________________________


volatile byte mypwm=0;
volatile byte ticks=0;
byte mode=1;
byte pmode=50;

byte eep[32];  //EEPROM buffer
byte eepos=0;

#ifdef RAMPING
  byte ramped=0;
#endif




void eepsave(byte data) {  //central method for writing (with wear leveling)
  byte oldpos=eepos;
  eepos=(eepos+1)&31;  //wear leveling, use next cell
  EEARL=eepos; EEDR=data; EECR=32+4; EECR=32+4+2;  //WRITE  //32:write only (no erase)  4:enable  2:go
  while(EECR & 2); //wait for completion
  EEARL=oldpos;           EECR=16+4; EECR=16+4+2;  //ERASE  //16:erase only (no write)  4:enable  2:go
}



inline void getmode(void) {  //read current mode from EEPROM and write next mode

  eeprom_read_block(&eep, 0, 32);  //+44                //read block 
  while((eep[eepos]==0xff) && (eepos<32)) eepos++; //+16   //find mode byte
  if (eepos<32) mode=eep[eepos];
  else eepos=0;   //+6  //not found

  byte next=1;
  if (mode==1) next=2;  //+10

#ifdef RAMPING  
  if (mode & 0x40) {   //RAMPING
    ramped=1;
    if (mode & 0x80)  { next=RAMPMODE+1; mode&=0x7f; } //mode: for savemode
    //else next=1
  }else              //ENDRAMPING
#endif

  if (mode & 0x80) {  //last on-time was short
    mode&=0x7f;   if (mode>=sizeof(modes)) mode=1;
    next=mode+1;  if (next>=sizeof(modes)) next=1; 
  } //else next=1;  //previous mode was locked, this one is yet a short-on, so set to restart from 1st mode.

  eepsave(next|0x80); //write next mode, with short-on marker
}





ISR(WDT_vect) {  //WatchDogTimer interrupt
if (ticks<255) ticks++;
if (ticks==16) eepsave(mode);  //lock mode after 2s
//code to check voltage and ramp down goes here: do ADC; average?; if (value<threshold) mypwm=(mypwm>>1)+3; 
}






int main(void) {

  portinit();
  sleepinit();
  ACoff;
  ADCoff;
  pwminit();

  getmode();  //get current mode number from EEPROM

  byte i=0;

#ifdef RAMPING
  byte p,j=0,dn=1;  //for ramping

  if (ramped) {  //use the ramped mode
     pmode=255; i=30-MINPWM-(mode&0x3f); while(i--){pmode=pmode-(pmode>>3)-1;}  //get PWM value  //never gives 250..254
  } else
#endif

  pmode=pgm_read_byte(&modes[mode]);  //get actual PWM value (or special mode code)


  switch(pmode){

    case 251:  mypwm=255; while(1){ PWM=mypwm; _delay_ms(20); PWM=0; _delay_ms(60); } break; //strobe 12.5Hz  //+48

    case 252:  mypwm=255; while(1){ PWM=mypwm; _delay_ms(20); PWM=0; i=70;do{SLEEP;}while(--i);  } break;  //beacon 10s //+48

#ifdef TIMER
    case 253:  i=TIMER; do{                   //+180
                 byte j=i; do{PWM=255; _delay_ms(20); PWM=0; _delay_ms(300); }while(--j); //blink remaining minutes   
                 byte k=30; do{ byte m=0; do{                                             //wait 1 minute
                   if (m<i) {_delay_ms(233);PWM=8;_delay_ms(90);PWM=0;}
                   else      _delay_ms(333);
                 }while(++m<6);    }while(--k);
               }while(--i);                                                               //for TIMER min
               i=100; do{ PWM=255; _delay_ms(30); PWM=0; _delay_ms(70); }while(--i);             //strobe 10s
               while(1){PWM^=8;SLEEP;}                                                           //"off"
               break;
#endif

#ifdef RAMPING
    case 254:  ticks=100; //crude way to deactivate the 2s-lock       //RAMPING
               while(EECR & 2); //wait for completion of getmode's write
               while(1){ p=255; i=30-MINPWM-j; while(i--){p=p-(p>>3)-1;}   //ramp up
                         PWM=p;
                         eepsave(192+j);    
                         SLEEP;
                         if (dn) {if (j          ) j--; else {dn=0;SLEEP;SLEEP;} } 
                         else    {if (j<30-MINPWM) j++; else {dn=1;SLEEP;SLEEP;} } //wear leveling
               } break;
#endif

    default:   mypwm=pmode; while(1){PWM=mypwm;SLEEP;}  //all other: us as PWM value

   //mypwm: prepared for being ramped down in WDT on low-batt condition.

  }//switch
  return 0;
}//main

