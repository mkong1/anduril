//v0 5/1/2014 by Everett
	//initial version
	//simple flashlight controller. mode change on power cycle
//v1 5/3/2014 by Everett
	//adapted to momentary switch
//v2 5/6/2014 by Everett
	//ported to Attiny10 device

#define F_CPU 1000000
#define pwm OCR0BL
#define pwm_invert
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>

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
char v_sample;
#define voltage_rate 100 //milliseconds

char pressed;
char new_press;
char switch_count;
unsigned char isr_prescale;

void configure(void);
unsigned char read_voltage(void);
void initialize_mode(void);
void debounce(void);
void shutdown(void);

ISR(TIM0_OVF_vect) //fires at 4kHz
{
	isr_prescale++;
	if(isr_prescale>=4){	//prescale since timer only has the 4kHz option
		isr_prescale=0;
		debounce();
		if(++v_timer==voltage_rate){v_timer=0; v_sample=1;}
	}
}

ISR(INT0_vect)
{
	//no action, but ISR must be included since it will execute upon waking
}


int main(void)
{
	configure();	//set up hardware peripherals
	
	mode=default_mode;
	initialize_mode();
	pressed=0; new_press=0; switch_count=10;
	sei();	//turn on interrupts

	while(1){
		
		if(v_sample){
			v_sample=0;
			if(mode==max){				//if battery goes below threshold in max mode, force down to medium mode
				if(read_voltage()<100){	//set threshold for external voltage here
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
			shutdown();	//shutdown will lock up here until a press wakes the device
		}
		
	}
}

void shutdown(void)
{
	TIMSK0=0;	//stop timer interrupt
	pwm=0;	//zero output
	TCCR0A=0; //turn off pwm
	#ifdef pwm_invert
	PORTB|=0b00000010;
	#else
	PORTB=0;		//ensure pin is low
	#endif
	ADCSRA=0;	//adc off
	PRR=3;	//power reduce on adc and timer0
	
	while(1)	//make this a loop so we stay here until sure the switch went down
	{
		do{
			debounce();
			_delay_ms((char)1);
		}while(pressed);	//ensure switch is up
		
		EICRA=0;	//interrupt on low level of int0
		EIMSK=1;	
		SMCR=0b00000101;	//enable sleep
		sleep_cpu();
		EIMSK=0;
		SMCR=0b00000100;	//disable sleep	
		
		pressed=0; switch_count=10;
		for(char i=0; i<40; i++){	//watch for up to 40ms for a solid press
			debounce();
			_delay_ms((char)1);
			if(pressed) break;	//if pressed break out of for loop
		}
		if(pressed) break;	//if pressed break out of sleep loop
	}
	
	configure();	//set up hardware for operation
	
}

void debounce(void)
{
	static char port_copy=0xff;
	#define switch_mask 0b00000001	//this selects PB0 as the switch
	
	if((PINB&switch_mask)==port_copy)	//if the current state matches previous state
	{
		if(--switch_count==0)	//count down samples. if 10 consecutive samples matched
		{
			switch_count=10;	//reset sample counter
			if(PINB&switch_mask) pressed=0;	//if the state is high, switch is up
			else		//else switch is down. check for new press
			{
				if(pressed==0) new_press=1;	//if last state of pressed was 0, this is a new press
				pressed=1;	//switch is now down
			}
		}
	}
	else	//state doesn't match,
	{
		switch_count=10;	//reset sample counter
		port_copy=(PINB&switch_mask);	//get new sample
	}
}

void initialize_mode(void)
{
	switch(mode){	//initialize current mode
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
	ADCSRA|=(1<<ADSC);	
	while(ADCSRA&(1<<ADSC));
	return ADCL;
}


void configure(void)
{
	PRR=0;
	TIMSK0=0b00000001;	//interrupt on t0 overflow
	pwm=0;
	#ifdef pwm_invert
	TCCR0A=0b00110001;	//output B inverted
	#else
	TCCR0A=0b00100001;	//output B on, not inverted, 8bit pwm,
	#endif
	TCCR0B=0b00001001;	//no prescale for 3906Hz pwm and interrupt
	GTCCR=0;
	
	PORTB=0;
	DDRB=0b00000010;	//PB1 output
	DIDR0=0b00000100;	//PB2 analog
	PUEB=0b11111001;	//pull up switch and reset

	ADMUX=2;	//PB2
	ADCSRA=0b10000011;	// 

	SMCR=0b00000100;	//enable power down mode
}
