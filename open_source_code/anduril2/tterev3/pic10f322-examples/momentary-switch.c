//v0 5/1/2014 by Everett
        //initial version
        //simple flashlight controller. mode change on power cycle
//v1 5/3/2014 by Everett
        //adapted to momentary switch

#include <htc.h>
#include <pic10f322.h>
#define _XTAL_FREQ 8000000
__CONFIG(CP_OFF & BOREN_OFF & LVP_OFF  & MCLRE_OFF & WDTE_OFF & PWRTE_OFF & FOSC_INTOSC & WRT_HALF);

#define pwm PWM1DCH

unsigned char mode;
enum mode{
        max=0,
        med=1,
        low=2,
        off=3,
};
#define max_mode 3
#define default_mode 0

unsigned int v_timer; 
bit v_sample;
#define voltage_rate 100 //milliseconds

bit pressed;
bit new_press;
char switch_count;

void delayms(int milliseconds);
void configure(void);
unsigned char read_voltage(void);
void initialize_mode(void);
void debounce(void);
void shutdown(void);

void interrupt isr(void)
{

        if(TMR0IF){     //fires at 1kHz
                TMR0IF=0;
                debounce();
                if(++v_timer==voltage_rate){v_timer=0; v_sample=1;}
        }
}


void main(void)
{
        configure();    //set up hardware peripherals
        
        mode=default_mode;
        initialize_mode();      
        pressed=0; new_press=0; switch_count=10;
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
                
                if(new_press){
                        new_press=0;
                        mode++;
                        if(mode>max_mode) mode=0;
                        initialize_mode();
                }
                
                if(mode==off)
                {
                        shutdown();     //shutdown will lock up here until a press wakes the device
                }                               
                
        }       
}

void shutdown(void)
{
        INTCON=0b00001000;      //interrupt on pin change only
        PWM1DCH=0;      //zero output
        PWM1CON=0;      //turn off pwm
        LATA=0;         //ensure pin is low
        FVRCON=0;       //fvr off
        ADCON=0;        //adc off
        
        while(1)        //make this a loop so we stay here until sure the switch went down
        {
                do{
                        debounce();
                        delayms(1);     
                }while(pressed);        //ensure switch is up
                
                IOCAP=0; 
                IOCAN=0b00001000;       //interrupt on fall of ra3
                IOCAF=0;        //clear flags
        
                SLEEP();
                
                pressed=0; switch_count=10;
                for(char i=0; i<40; i++){       //watch for up to 40ms for a solid press
                        debounce();
                        delayms(1);
                        if(pressed) break;      //if pressed break out of for loop
                }
                if(pressed) break;      //if pressed break out of sleep loop
        }               
        
        configure();    //set up hardware for operation
        GIE=1;  //turn on interrupts
        
}       

void debounce(void)
{
        static char port_copy=0xff;
        #define switch_mask 0b00001000  //this selects RA3 as the switch
        
        if((PORTA&switch_mask)==port_copy)      //if the current state matches previous state
        {
                if(--switch_count==0)   //count down samples. if 10 consecutive samples matched
                {
                        switch_count=10;        //reset sample counter
                        if(PORTA&switch_mask) pressed=0;        //if the state is high, switch is up
                        else            //else switch is down. check for new press
                        { 
                                if(pressed==0) new_press=1;     //if last state of pressed was 0, this is a new press
                                pressed=1;      //switch is now down
                        }
                }
        }
        else    //state doesn't match, 
        {
                switch_count=10;        //reset sample counter
                port_copy=(PORTA&switch_mask);  //get new sample
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
                case off:
                        pwm=0;
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
