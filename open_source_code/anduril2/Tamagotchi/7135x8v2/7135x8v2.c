/*
 * 7135x8 2.8A Nanjg 105C LED Driver
 *
 * Created: 23.01.2012 20:52:39
 * Last modified: 26.07.2013 02:57:00
 *
 * FIRMWARE VERSION: 2.6.5
 *
 * Fcpu = 4.8MHz
 *
 * This code is distributed under the GNU Public License
 * which can be found at http://www.gnu.org/licenses/gpl.txt
 *
 *  Author: Tamagotchi http://tamagotchi-007.livejournal.com/
 *                     http://avr.tamatronix.com
 *  Optimization: DooMmen http://doommen.blogspot.com/
 *				  vdavid http://forum.fonarevka.ru/member.php?u=20922
 *
 *  Download current version: http://109.87.62.61/uploads/LD_7135_v_2.6.5.zip
 *
 */

#include "7135x8v2.h"

register uint8_t short_off_counter		asm("r2");			/* WARNING */
register uint8_t adch_volt				asm("r3");			// напряжение
register uint8_t tick_volt				asm("r4");

register uint8_t current_mode			asm("r5");			/* WARNING */
register uint8_t mode_saver				asm("r6");			/* WARNING */
register uint8_t ramping_reg			asm("r7");			/* WARNING */
register uint8_t flag_reg				asm("r16");
//#define flag_reg						DIDR0

#ifdef THEMPERATURE_CONTROL
register uint8_t adch_temp				asm("r8");			// температура, нужно для калибровки
register uint8_t tick_temp				asm("r9");
register uint8_t calibrator10			asm("r10");			/* WARNING */
register uint8_t calibrator11			asm("r11");			/* WARNING */
register uint8_t MAX_THEMP				asm("r12");			// максимальная температура читается из EEPROM
#else
	#ifdef TURBO_TIME
	register uint8_t turbo_timer		asm("r8");
	register uint8_t prev_mode			asm("r9");
	#endif
#endif

#ifndef CAPACITOR
register uint8_t mode_switcher			asm("r13");
#endif

#ifndef NOMEM
#ifdef WEAR_LEWELING
uint8_t eeprom_pos = 0;
#endif
#endif


static NOINLINE void WriteEEPROMByte(uint8_t adress, uint8_t byte)
{
	while(EECR & _BV(EEPE));
	EEAR = adress;								// set address
	EEDR = byte;								// set data
	cli();										// disable interrupts
	EECR |= _BV(EEMWE);							// set "write enable" bit
	EECR |= _BV(EEWE);							// set "write" bit
	sei();
}

static NOINLINE uint8_t ReadEEPROMByte(uint8_t adress)
{
	while(EECR & 0x02);
	EEAR = adress;								// set address
	EECR |= _BV(EERE);							// set "read enable" bit
	return (EEDR);
}

#ifndef NOMEM
#ifdef WEAR_LEWELING
static INLINE void save_byte(uint8_t startadr, uint8_t data)
{
	uint8_t old_eeprom_pos = eeprom_pos;
	eeprom_pos=(eeprom_pos + 1) & 0x1F;
	WriteEEPROMByte((eeprom_pos + startadr), data);
	WriteEEPROMByte((old_eeprom_pos + startadr), 0xFF);
}

static NOINLINE uint8_t read_byte(uint8_t startadr)
{
	uint8_t eepdata;
	while(1)
	{
		eepdata = ReadEEPROMByte(eeprom_pos + startadr);
		if((eepdata != 0xFF) || (eeprom_pos >= SIZE))
			break;
		eeprom_pos++;
	}
	if (eeprom_pos < SIZE)
		return eepdata;
	else
		eeprom_pos = 0;
		return 0;
}
#endif
#endif

static INLINE void start_wdt(void)
{
	wdt_reset();
	WDTCR = (_BV(WDCE) | _BV(WDE));
	WDTCR = (_BV(WDTIE) | WDTO_250MS);
}

ISR(WDT_vect)
{
	ClearNewMode();
	short_off_counter = 0;
	#ifdef TURBO_TIME

		WDTCR = (_BV(WDCE) | _BV(WDE));
		WDTCR = (_BV(WDTIE) | WDTO_2S);

		if (turbo_timer) turbo_timer--;
		else
		#ifdef REVERSE_SW
			if (current_mode == MAX_MODES) current_mode--;
		#else 
			if (current_mode == 0) current_mode++;
		#endif
	#endif
}

static NOINLINE void delay_ms(uint8_t ms)	// оформляем так, чтобы меньше места занимало
{
	do { _delay_ms(1); }
	while(--ms);
}

static NOINLINE void delay_sec(uint8_t ms)
{
	do { _delay_ms(495); }
	while(--ms);
}

static NOINLINE void all_off(void)			// Выключаем вообще все
{
	LED_PORT = PB_CONFIG;
	OCR0A  = 0x00;
}

#ifndef FREQMOON_OFF
static NOINLINE void cpu_div(void)			// Устанавливаем делитель тактовой частоты
{
	CLKPR	= 0x80;
	CLKPR	= 0x02;
}
#endif

static NOINLINE void input_mode(void)
{
	all_off();
	delay_ms(150);
	mode_saver = 0xAA;
	#ifdef MODE_SET_LEVEL
		#if MAX_MODES == 3
			current_mode = ((current_mode - 1) & 0x03);		// ограничиваем счетчик режимов
		#else
			current_mode--;
			if (current_mode == 0xFF)
				current_mode = MAX_MODES;
		#endif
	#endif
}

static NOINLINE void pwr_down(void)
{
	cli();
	ADCSRA	= 0x00;	// отключаем ADC
	TCCR0A	= 0x00;	// отключаем таймер
	TCCR0B	= 0x00;	// отключаем таймер
	PRR		= 0x03;	// 0000 0011
	DDRB	= 0x00;	// отрубаем порт
	PORTB	= 0x00;
	MCUCR	= 0x30;	// PwrDown
	sleep_cpu();
}

#ifdef THEMPERATURE_CONTROL
static INLINE void switch_adc_in(uint8_t is_temp)
{
	if (is_temp) 			// если температура
		#ifdef THERMVD
			ADMUX = 0x62;	//  0110 0010	PB4 Vref = 1.1V
		#else
			ADMUX = 0x22;	//  0010 0010	PB4 Vref = Vcc
		#endif
	else
		ADMUX = 0x61;		//  0110 0001	PB2 Vref = 1.1V
}
#endif


ISR (ADC_vect)
{
	uint8_t adch_val = ADCH;

	#ifdef THEMPERATURE_CONTROL
	switch_adc_in(IsMeasureThemp());//

	if(IsMeasureThemp())
	{
		if (MAX_THEMP)											// Если 0 - отключаем термо защиту
		{
			#ifdef REVERSE_SW
			if(adch_val < (uint8_t)(MAX_THEMP + current_mode))  // режимы 0 - слабо 3 - сильно
			#else
			if(adch_val < (uint8_t)(MAX_THEMP - current_mode))  // режимы 0 - сильно 3 - слабо
			#endif
				tick_temp++;									// если температура велика - начинаем считать, чтобы убедиться, что это не помеха
			else
				tick_temp = 0;									// если температура нормализовалась - отбой
		}
		adch_temp = adch_val;
	}
	else
	{
	#endif
		if(adch_val < OFF_VOLTAGE)
			tick_volt++;							// если напруга мала - начинаем считать, чтобы убедиться, что это не помеха
		else
			tick_volt = 0;							// если напруга нормализовалась - отбой

		adch_volt = adch_val;

	#ifdef THEMPERATURE_CONTROL
	}
	InvMeasureMode();
	#endif

	#ifdef THEMPERATURE_CONTROL
	if((tick_volt > 0x40)|(tick_temp > 0x10))		// задержка для борьбы с помехами
	#else
	if(tick_volt > 0x80)
	#endif
	{
		tick_volt = 0;								// обнуляем, чтобы сбрасывать не в самый минимум, а порежимно
		#ifdef THEMPERATURE_CONTROL
		tick_temp = 0;
		#endif
		#ifndef REVERSE_SW
		current_mode++;								// уменьшаем яркость
		if (current_mode == (MAX_MODES+1))
			SetOffMode();
		#else
		current_mode--;								// здесь обратный порядок
		if (current_mode == 0xFF)
			SetOffMode();
		#endif
	}
}


#ifdef FLASH_OFF
static INLINE void flash(uint8_t i)
#else
static NOINLINE void flash(uint8_t i)
#endif
{
	all_off();
	do{	delay_ms(20);
		OCR0A ^= 0xFF;
		}while(--i);
}


static NOINLINE void set_mode(uint8_t mode)	 // сейчас верно для сверху вниз отключено SetMod3();
{
	switch (mode)
	{
		case 0:
			SetMod0();
			break;
		case 1:
			SetMod1();
			break;
		case 2:
			SetMod2();
			break;
		#if MAX_MODES >= 3
			case 3:
				SetMod3();
				break;
		#endif
		#if MAX_MODES == 4
			case 4:
				SetMod4();
				break;
		#endif
		default:
			pwr_down();
			break;
	}
	#ifdef TURBO_TIME
		if (mode != prev_mode)
		{
			#ifdef REVERSE_SW
				if (mode == MAX_MODES) turbo_timer=(TURBO_TIME/2);
			#else 
				if (mode == 0) turbo_timer=(TURBO_TIME/2);
			#endif
		}
	prev_mode=mode;
	#endif
}


uint8_t switch_voltage [] = {U1, U2, U3, U4};
static INLINE void display_voltage(void)
{
	uint8_t v_batt;
	uint8_t i = 0;
	input_mode();
	mode_saver = 0;
	v_batt = adch_volt;
	while(1)
	{
		OCR0A  = 0x80;
		delay_ms(75);
		OCR0A  = 0x00;
		delay_ms(100);
		if(v_batt < switch_voltage[i])
			break;
		if(++i >= 5)
			break;
	}
	delay_ms(100);

	#ifdef THEMPERATURE_CONTROL
	i = ReadEEPROMByte(CFG_CALIBRATE);
	if((!i) || (i == 0xFF))					 // если термоконтроль отключен - мигаем
		flash(10);
	#endif
}


#ifdef THEMPERATURE_CONTROL
static INLINE void calibrate(void)			 // калибровка термоконтроля
{
	input_mode();
	#ifndef FLASH_OFF
		flash(20);
	#endif
	#ifndef REVERSE_SW
		current_mode = 0;
	#else
		current_mode = MAX_MODES;
	#endif
	MAX_THEMP = 0;							 // обнуляем переменную т.е. отключаем ТК
	WriteEEPROMByte(CFG_CALIBRATE, 0);		 // обнуляем в eeprom
}
#endif


static INLINE void mode_line_chg(void)		 // включаем MODELINECHG
#ifdef ONECLICKMOON
{
	input_mode();
	WriteEEPROMByte(CFG_MOONMODE, 0x00);
	//ClearNewMode();
}
#else
{
	input_mode();
	uint8_t m = ReadEEPROMByte(CFG_MOONMODE);
	m ^= 0xFF;
	WriteEEPROMByte(CFG_MOONMODE, m);
	#ifdef RST_LEV_MOON
		#ifdef REVERSE_SW 					 // сброс яркости после мунлайта
			current_mode = 0;
		#else
			current_mode = MAX_MODES;
		#endif
	#endif
}
#endif


#ifdef RAMPING
static INLINE void ramping_loop(void)
{
	uint8_t k;
	input_mode();
	WriteEEPROMByte(CFG_RAMPING, 0);		 // обнуляем в eeprom
	#ifndef FREQMOON_OFF
		cpu_div();
	#endif
	while (1)
	{
		#ifdef FREQMOON_OFF
			ramping_reg = 2;
			k = 6;
		#else
			ramping_reg = 1;
			k = 7;
		#endif

		do
		{
			OCR0A = ramping_reg;
			#ifdef FREQMOON_OFF
				delay_ms(250);
			#else
				delay_ms(125);
			#endif
			if (IsOffMode())				 //для контроля батареи
				pwr_down();
			ramping_reg = (ramping_reg<<1);
		}while(--k);
	}
}
#endif


#ifdef VELO_STROBE
static INLINE void velo_pulse(void)
{
	input_mode();
	#ifndef MODE_SET_LEVEL
		current_mode = VeloMod;
	#endif
	if (VeloPulse == current_mode)
		current_mode--;
	while(1)
	{
		set_mode(VeloPulse); 
		delay_ms(25);
		set_mode(current_mode);
		delay_sec(VeloOFF);
	}
}
#endif


#ifdef POLICE_MODE
static INLINE void police_pulse(void)
{
	uint8_t k;
	input_mode();
	#ifndef MODE_SET_LEVEL
		current_mode = SlowPulseMod;
	#endif
	do{
		k = 4;
		do
		{
			set_mode(current_mode);
			delay_ms(5);
			all_off();
			delay_ms(50);
		}while(--k);
		delay_ms(200);
		delay_ms(140);
	}while(1);
}
#endif


#ifdef SOS_MODE
uint8_t sos_delay [] = {255, 75, 75, 75, 75, 75, 225, 225, 75, 225, 75, 225, 225, 75, 75, 75, 75, 75};
static INLINE void sos_pulse(void)	// это режим SOS
{
	uint8_t k;
	input_mode();
	#ifndef MODE_SET_LEVEL
		current_mode = SOSMod;
	#endif
	while(1)
	{
		k = 0;
		do
		{
			delay_ms(sos_delay[k]);
			k++;
			set_mode(current_mode);
			delay_ms(sos_delay[k]);
			k++;
			all_off();
		}while(k != 18);
		delay_ms(255);
	}
}
#endif


#ifdef PULSE_MODE
static INLINE void pulse(void)		// это частый антисобачий пульс (7.5Hz)
{
	input_mode();
	#ifndef MODE_SET_LEVEL
		current_mode = PulseMod;
	#endif
	while(1)
	{
		set_mode(current_mode);
		delay_ms(8);
		all_off();
		delay_ms(54);
	}
}
#endif


#ifdef SLOW_PULSE_MODE
static INLINE void s_pulse(void)	// этот медленный пульс, настраивается
{
	input_mode();
	#ifndef MODE_SET_LEVEL
		current_mode = SlowPulseMod;
	#endif
	while(1)
	{
		set_mode(current_mode);
		delay_ms(PULSE_ON);
		all_off();
		delay_sec(PULSE_OFF);
	}
}
#endif


#ifdef ALPINE_MODE
static INLINE void alpine(void)
{
	uint8_t i;
	input_mode();
	#ifndef MODE_SET_LEVEL
		current_mode = AlpineMod;
	#endif
	while(1)
	{
		i = 6;
		do
		{
			set_mode(current_mode);
			delay_ms(100);
			all_off();
			delay_sec(10);
		} while (--i);
		delay_sec(50);
	}
}
#endif


static INLINE void decode_mode(void)
{
	if (IsNewMode())												// если короткое отключение,...
	{
		if(!short_off_counter)
		{
			if(mode_saver == 0xAA)
			{
				mode_saver = 0;
				#ifndef	NOMEM
					#ifdef VELO_STROBE
						#ifdef WEAR_LEWELING
							current_mode = read_byte(CMODE_ADR);
						#else
							current_mode = ReadEEPROMByte(CFG_CURRENTMODE);
						#endif
					#endif
			//	#else
			//		current_mode--;
				#endif
			}
			else
				current_mode++;
		}
		short_off_counter++;
		if (short_off_counter == BATTERY_CLICKS)					//... проверяем сколько было коротких отключений
		{
			display_voltage();
		}
		#ifdef MODELINECHG
		else if (short_off_counter == MODELINECHG_CLICKS)
			mode_line_chg();
		#endif

		#ifdef THEMPERATURE_CONTROL
		else if (short_off_counter == CALIBRATE_CLICKS)
			calibrate();
		#endif

		#ifdef RAMPING
		else if (short_off_counter == RAMPING_CLICKS)
			ramping_loop();
		#endif

		#ifdef PULSE_MODE
		else if (short_off_counter == PULSE_MODE_CLICKS)
			pulse();
		#endif

		#ifdef SLOW_PULSE_MODE
		else if (short_off_counter == SLOW_PULSE_MODE_CLICKS)
			s_pulse();
		#endif

		#ifdef ALPINE_MODE
		else if (short_off_counter == ALPINE_MODE_CLICKS)
			alpine();
		#endif

		#ifdef VELO_STROBE
		else if (short_off_counter == VELO_STROBE_CLICKS)
			velo_pulse();
		#endif

		#ifdef POLICE_MODE
		else if (short_off_counter == POLICE_MODE_CLICKS)
			police_pulse();
		#endif

		#ifdef SOS_MODE
		else if (short_off_counter == SOS_MODE_CLICKS)
			sos_pulse();
		#endif
	}
	else
	{
		short_off_counter = 0;

	#ifndef	NOMEM												// если было длинное (включили) - читаем режим
		#ifdef WEAR_LEWELING
			current_mode = read_byte(CMODE_ADR);
		#else
			current_mode = ReadEEPROMByte(CFG_CURRENTMODE);
		#endif
			if (current_mode == 0xFF)							// на случай, если...
			current_mode = START_MODE;
	#else
		current_mode = START_MODE;								// обнуляем, чтобы всегда стартовать с минимума
	#endif
	}

	#if MAX_MODES == 3
		current_mode = current_mode & 0x03;						// ограничиваем счетчик режимов
	#else
		if (current_mode == 0xFF)
			current_mode = MAX_MODES;
		if (current_mode > MAX_MODES)
			current_mode = 0;
	#endif
}


static INLINE void initialize(void)
{
	CLKPR	= 0x80;
	CLKPR	= 0x01;			// устанавливаем делитель тактовой частоты

	// Port B initialization
	PORTB	= PB_CONFIG;	// PB4, PB5 подтягиваем к +
	DDRB	= DDRB_CONFIG;	// PB0, PB1, PB3 - выходы

	ACSR	= 0x80;			// 1000 0000  Analog Comparator Disable
	DIDR0	= 0x14;

	PRR		= 0;

	ADMUX	= 0x61;  		//  0110 0001 Internal Voltage Reference (1.1V) , ADC1 , 01 PB2 - напруга (или 10 PB4 - температура)
	ADCSRA	= 0xA7;  		//  0xA7 - 1010 1111 включаем, но не активируем, ADC Interrupt Enable, Division Factor = 128
	ADCSRB	= 0;

	//- включаем ШИМ
	TCNT0	= 0x00;
	OCR0A	= 0x00;
	TCCR0B	= 0x01;
	TCCR0A	= 0x83;

	MCUCR	= 0x20;			// IdleMode
	TIMSK0	= 0x00;			// timer interrupt sources
	GIMSK	= 0x00;			// interrupt sources

	sei();					// enable interrupts
}


static NOINLINE void std_loop(void)
{
	while(1)
	{
		set_mode(current_mode);
		if (IsNewMode())
		{
			#ifndef	NOMEM
				#ifdef WEAR_LEWELING
					delay_sec(1);
					save_byte(CMODE_ADR, current_mode);
				#else
					delay_sec(2);
					if((ReadEEPROMByte(CFG_CURRENTMODE)) != current_mode)
						WriteEEPROMByte(CFG_CURRENTMODE, current_mode);
				#endif
			#endif
		}
		#ifdef THEMPERATURE_CONTROL
			calibrator10 = adch_temp;
			calibrator11 = calibrator10;
		#endif
		delay_ms(3);
	}
}


static INLINE void moon_loop(void)				 // MOONMODE
{
	mode_saver = 0xAA;
	OCR0A = ramping_reg;

	#ifndef FREQMOON_OFF
		cpu_div();
	#endif
	while(1)
	{
		sleep_cpu();
		if (IsOffMode())
			pwr_down();
		delay_ms(1);
	}
}


OS_MAIN int main(void)
{
	cli();										 //disable interrupts
	ClearFlags();

	#ifdef CAPACITOR
		if((MCUSR & (1<<BORF)) != 0)
		{
			if((MCUSR & (1<<EXTRF)) == 0)	SetNewMode();
			MCUSR=0;
		}
	#else
		if (mode_switcher == 0xAA)
			SetNewMode();
		mode_switcher = 0xAA;
	#endif

	wdt_reset();
	tick_volt = 0;
	#ifdef THEMPERATURE_CONTROL
		tick_temp = 0;
		SetMeasureVolt();
	#endif

	#ifdef TURBO_TIME
		prev_mode=255;
	#endif

	initialize();
	start_wdt();

#ifdef THEMPERATURE_CONTROL
	uint8_t temporary = ReadEEPROMByte(CFG_CALIBRATE);
	if(!temporary)
	{
		if(IsNewMode())									// если короткий клик записываем новое значение ТК
		{
			if(calibrator11 == calibrator10)			// а вдруг?
			{
				#ifdef REVERSE_SW
					WriteEEPROMByte(CFG_CALIBRATE, calibrator10 - MAX_MODES);		 //
				#else
					WriteEEPROMByte(CFG_CALIBRATE, calibrator10);		 //
				#endif
				#ifndef FLASH_OFF
					flash(20);
				#endif
				pwr_down();
			}
		}
		else
			WriteEEPROMByte(CFG_CALIBRATE, 0xFF);		 // а если прерывание длинное - отключаем ТК
	}
	MAX_THEMP = temporary;
	if(MAX_THEMP == 0xFF) MAX_THEMP = 0;				 // на случай, если...
#endif

#ifdef RAMPING
	uint8_t  ramping = ReadEEPROMByte(CFG_RAMPING);
	if (!ramping)
	{
		if(IsNewMode())									 // если короткий клик записываем новое значение
		{
			WriteEEPROMByte(CFG_RAMPING, ramping_reg);
			WriteEEPROMByte(CFG_MOONMODE, 0x00);
			ClearNewMode();
		}
		else
			WriteEEPROMByte(CFG_RAMPING, 0xFF);
	}
	else
		ramping_reg = ramping;

	if (ramping_reg == 0xFF)
		#ifdef FREQMOON_OFF
			ramping_reg = 2;
		#else
			ramping_reg = 1;
		#endif
#else
	#ifdef FREQMOON_OFF
		ramping_reg = 2;
	#else
		ramping_reg = 1;
	#endif
#endif

	adc_on();

	decode_mode();

#ifndef MODELINECHG
		std_loop();
#else
	if (ReadEEPROMByte(CFG_MOONMODE) == 0xFF)
		std_loop();
	else
		#ifdef ONECLICKMOON
			if(IsNewMode())							 // если короткий клик выходим из MODELINECHG
			{
				WriteEEPROMByte(CFG_MOONMODE, 0xFF);
				#ifdef RST_LEV_MOON
					#ifdef REVERSE_SW 				 // сброс яркости после мунлайта
						current_mode = 0;
					#else
						current_mode = MAX_MODES;
					#endif
				#else
					ClearNewMode();
				#endif
				std_loop();
			}
			else
				moon_loop();
		#else
			moon_loop();
		#endif
#endif
}

