//NANJG driver, dynamic heartbeat mode

#define F_CPU 4800000 //use fuses low:0x75 high:0xff
#include <avr/io.h>
#include <util/delay.h>

uint8_t rand() { static uint16_t random=1; random=(random>>1)^(-(random&1u)&0xB400); return random>>8; }

int main() {
  DDRB=2; //define PB1 as output
  TCCR0A=0b00100001; TCCR0B=0b00000001; //PWM setup, 9kHz
  uint8_t tic=0, ticlen=100,ticadd=10,beat=0; //ticlen=100 gives a beat of 1s
  while(1) { //endless loop
    if (tic==0) {
      //the dynamics go here... decide about this beat's length
      beat++;
      ticlen+=ticadd;  //make smooth transitions
      if (beat==5) {beat=0; ticadd=18+rand()%18-ticlen/5;}
    }
    tic++;
    if (tic<=18) OCR0B=14*tic;  //double pulse
    else if (tic<=27) OCR0B=14*(36-tic);
    else if (tic<=36) OCR0B=14*(tic-18);
    else if (tic<=54) OCR0B=14*(54-tic);
    else if (tic<=100) OCR0B=0;
    else tic=0;
    for (uint8_t i=0;i<ticlen;i++) _delay_us(100);
  }
}
