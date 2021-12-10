//
// LuxDrv 0.30b    DrJones 2011++
//
// 808 bytes with ramping
//
// License: CC-BY-NC-SA (non-commercial use only, derivative works must be under the same license)
//
// changes:
// 0.1  first version: levels,strobe,beacon,timer(136)   (638bytes)
// 0.2  ramping(200), improved timer(180), skip mode1 as next mode if already in mode 1     (898bytes)
// 0.21 changed EEPROM handling to reduce power-off-while-writing glitches -->eepsave(byte)  (874bytes)
// 0.22 bugfix in ramping; mode not stored on (very) short tap   (878bytes)
// 0.3  battery monitoring added; step down and indicator in beacon mode (994bytes)
// 0.3b improved config, const progmem, no dummy any more,  timer removed, 4 main modes as default

//ToDo: 
//  ? off-time checking; requires adding  diode,R,C
//  ? blink your name in morse code :)


#define F_CPU 4800000    //CPU: 4.8MHz  PWM: 9.4kHz       ####### use low fuse: 0x75  #######   
//########################################################################################### MODE/PWM SETUP 
//Special modes; comment out to disable
#define RAMPING   254  //+156
#define STROBE    253  //+50
#define BEACON    252  //+74


#define MODES      6,15,56,255,   RAMPING,  STROBE,  BEACON

#define RAMPMODE 5  //the number of the RAMPING mode in the MODES order; e.g. RAMPING is 5th mode -> 5


//define NOMEM         //deactivate mode memory.
#define LOCKTIME 7   //time in 1/8 s until a mode gets locked, e.g. 12/8=1.5s
#define BATTMON  125 //enable battery monitoring with this threshold
#define MINPWM   5   //needed for ramping
//###########################################################################################


#define outpin 1
 #define PWM OCR0B  //PWM-value
#define adcpin 2
 #define adcchn 1
#define portinit() do{ DDRB=(1<<outpin); PORTB=0xff-(1<<outpin)-(1<<adcpin);  }while(0)

#include <avr/pgmspace.h>
#define byte uint8_t
#define word uint16_t

PROGMEM byte modes[]={ MODES };

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>



#define WDTIME 0b01000011  //125ms

#define sleepinit() do{ WDTCR=WDTIME; sei(); MCUCR=(MCUCR &~0b00111000)|0b00100000; }while(0) //WDT-int and Idle-Sleep

#define SLEEP asm volatile ("SLEEP")

#define pwminit() do{ TCCR0A=0b00100001; TCCR0B=0b00000001; }while(0)  //chan A, phasePWM, clk/1  ->2.35kHz@1.2MHz

#define adcinit() do{ ADMUX =0b01100000|adcchn; ADCSRA=0b11000100; }while(0) //ref1.1V, left-adjust, ADC1/PB2; enable, start, clk/16
#define adcread() do{ ADCSRA|=64; while (ADCSRA&64); }while(0)
#define adcresult ADCH


#define ADCoff ADCSRA&=~(1<<7) //ADC off (enable=0);
#define ADCon  ADCSRA|=(1<<7)  //ADC on
#define ACoff  ACSR|=(1<<7)    //AC off (disable=1)
#define ACon   ACSR&=~(1<<7)   //AC on  (disable=0)


//_____________________________________________________________________________________________________________________


volatile byte mypwm=0;
volatile byte ticks=0;
volatile byte mode=0;
byte pmode=50;

byte eep[32];  //EEPROM buffer
byte eepos=0;

#ifdef RAMPING
  byte ramped=0;
#endif

byte lowbattcounter=0;



void eepsave(byte data) {  //central method for writing (with wear leveling)
  byte oldpos=eepos;
  eepos=(eepos+1)&31;  //wear leveling, use next cell
  EEARL=eepos; EEDR=data; EECR=32+4; EECR=32+4+2;  //WRITE  //32:write only (no erase)  4:enable  2:go
  while(EECR & 2); //wait for completion
  EEARL=oldpos;           EECR=16+4; EECR=16+4+2;  //ERASE  //16:erase only (no write)  4:enable  2:go
}




ISR(WDT_vect) {  //WatchDogTimer interrupt
  if (ticks<255) ticks++;
  if (ticks==LOCKTIME)
  #ifdef NOMEM
    eepsave(0);  //current mode locked -> next time start over, no memory.
  #else 
    eepsave(mode);
  #endif


  #ifdef BATTMON //code to check voltage and ramp down
   adcread(); 
   if (adcresult<BATTMON) { if (++lowbattcounter>8) {mypwm=(mypwm>>1)+3;lowbattcounter=0;} } 
   else lowbattcounter=0;
  #endif
}




inline void getmode(void) {  //read current mode from EEPROM and write next mode

  eeprom_read_block(&eep, 0, 32);                     //read block 
  while((eep[eepos]==0xff) && (eepos<32)) eepos++;    //find mode byte
  if (eepos<32) mode=eep[eepos];
  else eepos=0;

  byte next=0;
  if (mode==0) next=1; //skip 1st mode if memory is 1st mode already

  #ifdef RAMPING  
   if (mode & 0x40) {   //RAMPING
     ramped=1;
     if (mode & 0x80)  { next=RAMPMODE; mode&=0x7f; } //mode: for savemode
   }else              //ENDRAMPING
  #endif

  if (mode & 0x80) {  //last on-time was short
    mode&=0x7f;   if (mode>=sizeof(modes)) mode=0;
    next=mode+1;  if (next>=sizeof(modes)) next=0; 
  } 

  eepsave(next|0x80); //write next mode, with short-on marker
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

    #ifdef RAMPING
    case RAMPING: 
      ticks=100; //crude way to deactivate the 2s-lock       //RAMPING
      while(EECR & 2); //wait for completion of getmode's write
      while(1){ p=255; i=30-MINPWM-j; while(i--){p=p-(p>>3)-1;}   //ramp up
                PWM=p;
                eepsave(192+j);    
                SLEEP;
                if (dn) {if (j          ) j--; else {dn=0;SLEEP;SLEEP;} } 
                else    {if (j<30-MINPWM) j++; else {dn=1;SLEEP;SLEEP;} }
      } break;
    #endif

    #ifdef STROBE
    case STROBE:  mypwm=255; while(1){ PWM=mypwm; _delay_ms(20); PWM=0; _delay_ms(60); } break; //strobe 12.5Hz  //+48
    #endif

    #ifdef BEACON
    case BEACON:  
      #ifdef BATTMON    
        adcread(); i=adcresult; while (i>BATTMON) {PWM=8; SLEEP;SLEEP; PWM=0; SLEEP; i-=5;}   SLEEP;SLEEP;SLEEP;
      #endif    
      mypwm=255; while(1){ PWM=mypwm; _delay_ms(20); PWM=0; i=70;do{SLEEP;}while(--i);  } break;  //beacon 10s //+48
    #endif


    default:   mypwm=pmode; while(1){PWM=mypwm;SLEEP;}  //all other: us as PWM value

  }//switch
  return 0;
}//main

