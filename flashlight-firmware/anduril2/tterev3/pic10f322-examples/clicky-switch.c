//v0 5/1/2014 by Everett
        //initial version
        //simple flashlight controller. mode change on power cycle
        

#include <htc.h>
#include <pic10f322.h>
#define _XTAL_FREQ 8000000
__CONFIG(CP_OFF & BOREN_OFF & LVP_OFF  & MCLRE_ON & WDTE_OFF & PWRTE_OFF & FOSC_INTOSC & WRT_HALF);

#define pwm PWM1DCH

persistent unsigned char mode;  //must be declared persistent for ram retention trick to work
enum mode{
        max=0,
        med=1,
        low=2,
};
#define max_mode 2
#define default_mode 0

persistent unsigned int key;
unsigned int v_timer; 
bit v_sample;
#define voltage_rate 100 //milliseconds

void delayms(int milliseconds);
void configure(void);
unsigned char read_voltage(void);
void initialize_mode(void);

void interrupt isr(void)
{

        if(TMR0IF){     //fires at 1kHz
                TMR0IF=0;
                if(++v_timer==voltage_rate){v_timer=0; v_sample=1;}
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

        initialize_mode();      
        GIE=1;  //turn on interrupts

        while(1){
                
                if(v_sample){
                        v_sample=0;
                        if(mode==max){                          //if battery goes below 3.0V in max mode, force down to medium mode
                                if(read_voltage()>87){
                                        mode=med;
                                        initialize_mode();
                                }
                        }
                }                       
                
        }       
}

void initialize_mode(void)
{
                switch(mode){   //initialize current mode
                default:
                case max:
                        pwm=255;
                        break;
                case med:
                        pwm=25;
                        break;
                case low:
                        pwm=1;
                        break;
        }       
}       

unsigned char read_voltage(void)        
{
//fvr is at 1.024V. ADRES = 1.024/Vin*255. Vin = 1.024/(ADRES/255). voltage limit set to 3.0V -> 87. values below this correspond to voltage above 3.0V
        GO_nDONE=1;
        while(GO_nDONE);
        return ADRES;
}

        
void delayms(int milliseconds)
{
        while(milliseconds!=0){ __delay_ms(1); milliseconds--;}
}

void configure(void)
{
        INTCON=0b00100000;      //tmr0 only

        T2CON=0b00000100; //on, no prescale
        PR2=255; 
        TMR2=0;
                
        LATA=0;
        TRISA=0b11111110;       //GP0 output
        ANSELA=0b11110000;      //
        WPUA=0b11111110;

        OSCCON=0b01100000;      //8MHz

        OPTION_REG=0b00000010;  //8 prescale for 1ms interrupts

        FVRCON=0b10000001;      //1.024v to adc
        ADCON=0b10111101;       // fvr

        PWM1DCH=0;
        PWM1CON=0b11000000;     //on, output
}

 
