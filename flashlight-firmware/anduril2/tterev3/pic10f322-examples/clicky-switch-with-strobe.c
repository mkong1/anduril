//v0 4/21/2014 by Everett
        //initial version
        //simple flashlight controller. mode change on power cycle
        

#include <htc.h>
#include <pic12f1822.h>

#define _XTAL_FREQ 8000000

__CONFIG(FOSC_INTOSC & WDTE_SWDTEN & PWRTE_ON & MCLRE_ON & CP_OFF & CPD_OFF & BOREN_ON & CLKOUTEN_OFF & IESO_OFF & FCMEN_OFF);
__CONFIG(WRT_BOOT & PLLEN_OFF & STVREN_ON & BORV_LO & LVP_OFF);


persistent unsigned char mode;  //must be declared persistent for ram retention trick to work
enum mode{
        max=0,
        med=1,
        low=2,
        strobe=3,
};
#define max_mode 3
#define default_mode 0

#define on_time 15      //milliseconds

unsigned int strobe_timer;
unsigned char strobe_position;
bit on_off;
persistent unsigned int key;

void delayms(int milliseconds);
void configure(void);
unsigned char stun_rate_lookup(unsigned char input);

void interrupt isr(void)
{
        if(TMR1IF){
                TMR1IF=0;
                }

        if(TMR0IF){     //fires at 1kHz
                TMR0IF=0;
                if(strobe_timer){strobe_timer--;}       //count down milliseconds
        }
}




void main(void)
{
        configure();    //set up hardware peripherals
        
        delayms(15);    //short delay to avoid power glitches incrementing mode
        
        if(key==12345){         //RAM retention trick to detect quick power cycles
                mode++;                 //go to next mode
                if(mode>max_mode){mode=0;}
        }       
        else{                   //long power loss. default to first mode
                mode=default_mode;
                key=12345;
        }               


        switch(mode){   //initialize current mode
                default:
                case max:
                        CCPR1L=255;
                        break;
                case med:
                        CCPR1L=25;
                        break;
                case low:
                        CCPR1L=1;
                        break;
                case strobe:
                        strobe_timer=0;
                        on_off=0;
                        CCPR1L=0;
                        break;
        }               

        
        GIE=1;  //turn on interrupts

        while(1){
                
                if(mode==strobe){       //no other modes need active tasks while running
                        if(strobe_timer==0){    //timer ran out
                                on_off=~on_off; //flip it
                                if(on_off){
                                        CCPR1L=255;
                                        strobe_timer=on_time;   //set on time
                                }
                                else{
                                        CCPR1L=0;
                                        strobe_position++;
                                        strobe_timer=stun_rate_lookup(strobe_position); //set off time
                                }       //set output
                        }       
                }
                
                
                
        }       
}


unsigned char stun_rate_lookup(unsigned char input)
{
        static const char table[72]=28,30,33,31,12,24,28,33,29,23,19,33,33,23,14,26,9,18,15,29,15,10,19,31,9,27,33,10,15,26,16,28,30,24,14,23,10,24,30,10,23,31,25,33,9,15,19,17,19,18,32,23,33,17,31,13,31,18,20,8,24,33,17,21,20,14,9,20,26,16,22,16;
        if(input>71){input-=71;}
        if(input>71){input-=71;}
        if(input>71){input-=71;}
        return table[input];    
}       



void delayms(int milliseconds)
{
        while(milliseconds!=0){ __delay_ms(1); milliseconds--;}
}



void configure(void)
{
        INTCON=0b01100000;
        PIR1=0;
        PIR2=0;
        T1CON=0b00110001;
        T2CON=0b00000101; 
        PR2=255; 
        TMR2=0;
                
        LATA=0;
        TRISA=0b11111011;
        ANSELA=0b00000000;      //
        WPUA=0b11111011;
        APFCON=0;

        OSCCON=0b01110011;      //8MHz
        PIE1=0b00000000;
        PIE2=0;
        OPTION_REG=0b00000010;  //8 prescale for 1ms interrupts

//      FVRCON=0b11000001;      //1.024v to adc
//      ADCON0=0b00001101;      //
//      ADCON1=0b00010000;      //1/8, left justify, vref from vdd

        CCPR1L=0;
        CCP1CON=0b00001100;
}


__EEPROM_DATA(0,0,0,0,0,0,0,0); 

__EEPROM_DATA(0, 0, 0, 0, 0, 0, 0, 0);
__EEPROM_DATA(0, 0, 0, 0, 0, 0, 0, 0);

__EEPROM_DATA('1','8','2','2',' ','f','l','a');
__EEPROM_DATA('s','h','l','i','g','h','t',' ');
__EEPROM_DATA('b','y',' ','E','v','e','r','e');
__EEPROM_DATA('t','t',' ','e','v','e','r','e');
__EEPROM_DATA('t','t','.','b','r','a','d','f');
__EEPROM_DATA('o','r','d','@','g','m','a','i');
__EEPROM_DATA('l','.','c','o','m',' ',' ',' ');
