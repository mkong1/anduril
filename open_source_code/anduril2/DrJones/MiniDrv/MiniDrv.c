//MiniDrv -- minimalistic driver firmware (no blinkies)  --  DrJones 2014
#define F_CPU 4800000                    //use fuses  low:0x75  high:0xff
#include <avr/io.h>
#include <util/delay.h>
//change modes here; just change/add/delete values
uint8_t modes[]={8,90,255};              //PWM values, 5..255
int main() {
    DDRB=2;                                //define PB1 as output
    TCCR0A=0b00100001; TCCR0B=0b00000001;  //PWM setup, 9kHz
    EEARL=4;EECR=1; uint8_t mode=EEDR;     //read from EEPROM
    if (mode>=sizeof(modes)) mode=0;       //check if invalid
    OCR0B=modes[mode];                     //set PWM level
    EEDR=mode+1;                           //next mode
    if(EEDR>=sizeof(modes)) EEDR=0;
    EECR=4;EECR=4+2;  while(EECR&2);       //write to EEPROM
    _delay_ms(1000);                       //delay for memory (or nomemory) to kick in
    EEDR=mode;                             //memory: use this mode again   \  use one of these lines
    //EEDR=0;                                //no memory: restart from 0   /  use one of these lines
    EECR=4;EECR=4+2;  while(EECR&2);       //write to EEPROM
    while(1) {}                            //endless loop
}
