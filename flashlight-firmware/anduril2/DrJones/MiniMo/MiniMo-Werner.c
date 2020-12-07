//MiniMo -- minimalistic driver firmware for momentary buttons (no blinkies)  --  DrJones 2014
//Up&Down variant ("Werner's UI")
#define F_CPU 4800000                    //use fuses  low:0x75  high:0xff
#include <avr/io.h>
#include <util/delay.h>
//change modes here; just change/add/delete values. The "0" is 'off' 
uint8_t modes[]={0,  8,90,255};          //PWM values, 5..255 - LEAVE THE "0" THERE
int main() {
  DDRB=2; PORTB=8;                       //define PB1 as output and pull-up switch on PB3
  TCCR0A=0b00100001; TCCR0B=0b00000001;  //PWM setup, 9kHz
  uint8_t count=0,mode=0,waspressed=0;   //define some variables used below

  while(1) {                             //endless loop

    if ((PINB&8)==0) {                   //when the button is pressed (PB3 pulled down)
      count++;                           //count length of button press
      if (count==16) {                   //pressed long (16*25ms=0.4s)
        if (mode>0)  mode--;             // ** decrease mode
        else mode=sizeof(modes)-1;       // ** wraparound
      }
      waspressed=1;                      //remember that the button was pressed, see below
    }
    else {                               //button not pressed
      if (waspressed) {                    //but it was pressed, so it has just been released!
        waspressed=0;                      //reset that
        if (count<16) {                  // ** really was a short press
          mode++; if (mode>=sizeof(modes)) mode=0; // ** next mode
        }
        count=0;                           //reset counter
      }
    }    


  OCR0B=modes[mode];                     //set PWM level (0 is off)
  _delay_ms(25);                         //wait a bit before checking again, important for counting
  }
}
