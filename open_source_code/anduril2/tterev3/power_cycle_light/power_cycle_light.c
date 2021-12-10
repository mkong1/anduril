//v0 9/9/2014 by Everett
	//initial version
	//simple flashlight controller. mode change on power cycle

#include <htc.h>
#include <pic10f322.h>
#define _XTAL_FREQ 8000000
__CONFIG(CP_OFF & BOREN_OFF & LVP_OFF  & MCLRE_OFF & WDTE_OFF & PWRTE_OFF & FOSC_INTOSC & WRT_HALF);

#define pwm PWM1DCH

persistent unsigned char mode;
enum mode{
	max=0,
	low,
	ramp,
	ramp_save,
};
#define max_mode 1
#define default_mode 0

#define safe_level 63	//25%

#define ramp_rate 15	//4 second ramp
#define ramp_save_min 33	//minimum time in ramp before it will be saved. in counts of ramp_rate (33*15=500ms)
#define ramp_pause 500	//milliseconds


persistent unsigned int password;
persistent unsigned char cycle_count;
persistent unsigned char level;

unsigned int ramp_save_timer;
unsigned int ramp_timer;
unsigned int v_timer; 
bit v_sample;
#define voltage_rate 50 //milliseconds

persistent unsigned int on_timer;

bit direction;

#define storage_location 0x1f0
const unsigned char nvm[16] @storage_location={100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void delayms(int milliseconds);
void configure(void);
unsigned char read_voltage(void);
void initialize_mode(void);
void save_level(unsigned char input);
unsigned char read_level(void);
unsigned char lookup(unsigned char input);

void interrupt isr(void)
{

	if(TMR0IF){	//fires at 1kHz
		TMR0IF=0;
		if(++v_timer==voltage_rate){v_timer=0; v_sample=1;}
		if(ramp_timer){ramp_timer--;}
		if(on_timer){
			on_timer--;
			if(on_timer==0)cycle_count=0;
		}
	}
}


void main(void)
{
	configure();	//set up hardware peripherals

	delayms(25);

	if(password==12345){
		if(mode==ramp_save){
			save_level(level);
			mode=low;
			cycle_count=0;
		}
		else{	
			cycle_count++;
			if(on_timer) mode++;
			if(mode>max_mode) mode=0;
			if(cycle_count>=8){
				mode=ramp;
				ramp_save_timer=0;
			}	
		}	
	}
	else{
		cycle_count=0;
		mode=default_mode;
		password=12345;
	}	
	initialize_mode();	
	on_timer=400;	//if left on longer than this, stop counting cycles to enter ramp

	GIE=1;	//turn on interrupts

	while(1){
		
		
		switch(mode){
			case max:
			case low:
				if(v_sample){
					v_sample=0;
						if(read_voltage()>87){
							if(pwm>safe_level) pwm--;
						}
				}	
				break;
			case ramp:
			case ramp_save:
				if(ramp_timer==0){
					ramp_timer=ramp_rate;
					if(++ramp_save_timer>ramp_save_min) mode=ramp_save;
					if(direction){
						level++;
						if(level==255){ direction=0; ramp_timer=ramp_pause;}
					}
					else{
						level--;
						if(level==0){ direction=1; ramp_timer=ramp_pause;}
					}
					pwm=lookup(level);
				}
				break;
		}					
			
	}	
	
}


unsigned char lookup(unsigned char input)
{
	unsigned int temp = input*input;
	temp>>=8;
	return (temp&0xff)+1;
}	


void initialize_mode(void)
{
		switch(mode){	//initialize current mode
		default:
		case max:
			pwm=255;
			break;
		case low:
			pwm=lookup(read_level());
			break;
		case ramp:
			level=0;
			pwm=lookup(level);
			direction=1;
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
	INTCON=0b00100000;	//tmr0 only

	T2CON=0b00000100; //on, no prescale
	PR2=255; 
	TMR2=0;
		
	LATA=0;
	TRISA=0b11111110;	//GP0 output
	ANSELA=0b11110000;	//
	WPUA=0b11111110;

	OSCCON=0b01100000;	//8MHz

	OPTION_REG=0b00000010;	//8 prescale for 1ms interrupts

	FVRCON=0b10000001;	//1.024v to adc
	ADCON=0b10111101;	// fvr

	PWM1DCH=0;
	PWM1CON=0b11000000;	//on, output
}

void save_level(unsigned char input)
{
	GIE=0;
	PMADR=storage_location;
	PMCON1=0b00010100;	//FREE and WREN
	PMCON2=0x55; PMCON2=0xAA;
	#asm
	bsf	PMCON1,1
	nop
	nop
	#endasm		//erased	
	PMADR=storage_location;
	PMCON1=0b00100100;	//LWLO and WREN
	PMDATH=0x34;
	PMDATL=input;
	PMCON2=0x55; PMCON2=0xAA;
	#asm
	bsf	PMCON1,1
	nop
	nop
	#endasm		//write to latch
	PMCON1=0b00000100;	//WREN
	PMCON2=0x55; PMCON2=0xAA;
	#asm
	bsf	PMCON1,1
	nop
	nop
	#endasm		//write
	PMCON1=0;	//clear WREN
	GIE=1;
}
	
unsigned char read_level(void)
{
	PMADR=storage_location;
	PMCON1=0b00000001; //RD
	#asm
	nop
	nop
	#endasm
	return PMDATL;
}	