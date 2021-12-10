/*
 * GXB172 Rev A - Single-Cell ~50W 17mm Boost LED Flashlight Driver
 * Copyright (C) 2018 Gao Guangyan
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * Firmware version 1.0 (27 Mar 2018) for ATtiny841
 *  - For more information: www.loneoceans.com/labs/gxb172/
 * 	- by Gao Guangyan, contact loneoceans@gmail.com
 *   
 * NOTE - THIS FIRMWARE IS A CONTINUAL WORK IN PROGRESS! 
 * NOTE - Values used in this example work for a Convoy S2+ Host
 * 
 * Code written in Arduino IDE for ease of use among hobbyists
 * 
 * 	- Complied with Arduino 1.8.5 with ATtinyCore Board Library V1.1.4
 * 	   (http://highlowtech.org/?p=1695)
 * 	- Select Tools>Board: ATTiny441/ATTiny841 (no bootloader) with 8MHz internal crystal
 * 	- Board reference https://github.com/SpenceKonde/ATTinyCore
 *     (Currently using V1.1.4 of ATTinyCore)
 *     Settings: Board Attiny441/841 no bootloader, BOD LTO Disabled, ATtiny841, Clk 8MHz internal
 *	- Credit for Felias-fogg's SoftI2CMaster 
 *	- Credit for PID Library V1.2.1 by Brett Beauregard
 *     PID files need to be in the same directory during compile
 *     Some parts not required removed from code to reduce compile size
 * 
 * Complie using Arduino IDE (https://forum.arduino.cc/index.php?topic=131655.0)
 * Flash 841 using AVR ISP2 or Atmel ICE or similar (e.g. via Atmel Studio 7).
 * 
 * 	- Set fuses to Internal 8MHz Oscillator, Low.CKDIV8 unchecked.
 * 	- Fuses are: 0xFF,DF,C2 (EXT, HI, LOW)
 *  
 *  "Sketch uses 6852 bytes (83%) of program storage space.
 *   Global variables use 128 bytes (25%) of dynamic memory, leaving 384 bytes for local variables."
 *  To complie, enable Verbose Output, click Verify in Arduino IDE. Get .hex from displayed dir. 
 */

#define F_CPU 8000000
#define ADDRLEN 1
#define I2C_TIMEOUT 1000
#define SDA_PORT PORTA
#define SDA_PIN 6
#define SCL_PORT PORTA
#define SCL_PIN 4

#include <SoftI2CMaster.h>
#include <EEPROM.h>
#include "PID_v1.h"

// #######################################
// ##          SET DEFINITIONS          ##
// #######################################

// Pin Definitions (Based on Arduino IDE)

const uint8_t Pin_PWM = 3;    	// PA7 PP15 OC2B
const uint8_t Pin_LedDbg = 7; 	// PA3 PP2
const uint8_t Pin_Enable = 10;	// PA0 PP5
const uint8_t Pin_Batt = 1;    	// PA1 ADC1 PP4
const uint8_t Pin_OTCA = 2;    	// PA2 ADC2 PP3
const uint8_t Pin_OTCD = 8;    	// PA2 ADC2 PP3
const uint8_t Pin_PB1 = 1;     	// PB1 PP12
const uint8_t Pin_PB0 = 0;     	// PB0 PP11

uint8_t bVreadings = 0;
uint8_t runningPID = 0;
int currentBrightness = 0;		// the current LED brightness out of 1023

// Flashlight brightness States
int brightnessValues[5] = {0,-1,45,182,852}; // Default - Modify if desired

uint8_t NUMSTATES = 5;        // Modify if desired
uint8_t currentState = 0;
uint8_t olderState = 128;     // Init with a big number at first, will be updated when program is run
uint8_t memory = 1;           // If memory mode is on or not. 1 = ON by default
uint8_t runningCandlelight;
// runningCandlelight is set to 0 during state changes so we can get out of candlelightmode.

// Variables for Mode Memory and OTC

volatile int eepos = 0;
volatile int oldpos;

#define EEPLEN          255   // Modify this depending on micro used, some only have 128 bytes
#define CAP_SHORT       250   // Short Press  (modify depending on OTC cap), ADC val out of 1024
#define CAP_LONG        100   // Medium Press (modify depending on OTC cap), functionality not used yet..

// Define Low Voltage Values
// Battery is read across 1kR low across 5.7kR network at VCC=2.5V.

#define LOW_BATTERY       210   // ~2.93V
#define CRITICAL_BATTERY  193   // ~2.69V

// Define TMP103 Values

#define TMP103G 0x76
#define TEMPERATURE_REGISTER 0x00
#define MAX_TEMPERATURE 128
#define MIN_TEMPERATURE -55 

double SET_TEMPERATURE = 60;    // PID will regulate at this temperature
int CRITICAL_TEMPERATURE = 90;	// If temp exceeds this, shut down flashlight


// Define PID Variables

double Setpoint, Input, Output, OldOutput;
double Kp=1, Ki=1, Kd=1;
PID tempPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
#define PID_LOW_LIMIT 60  		// Adjust to prevent too much undershoot

// #######################################
// ##     METHODS FOR EEPROM MEMORY     ##
// #######################################

// Wear leveling adapted from JCapSolutions STAR_off_time.c
// Note - be sure to clear EEPROM before using

void save_state(){
  uint8_t eep;
  oldpos = eepos;
  eepos = (eepos+1);
  if (eepos > EEPLEN){
    eepos = 0;
  }
  eep = currentState;
  EEPROM.write(eepos,eep);
  EEPROM.write(oldpos,0xff);
}

void restore_state(){
  // Find the saved state from the EEPROM and restore
  uint8_t eep;
  for (int p=0; p<EEPLEN; p++){
    eep = EEPROM.read(p);
    if (eep!=0xff){
      eepos = p;
      currentState = eep;
      break;
    }
  }
  if (currentState >= NUMSTATES){
    currentState = 0;
  }
}

void next_state(){
  bVreadings = 0;
  currentState += 1;
  if (currentState >= NUMSTATES){
    currentState = 0;
  }
  save_state();
}

void drop_state(){
  bVreadings = 0;
  currentState -= 1;
  if (currentState<0){
    currentState = 0;
  }
  save_state();
}

// #######################################
// ##      METHODS FOR TMP103 READ      ##
// #######################################

uint8_t read_register(uint8_t reg){
  uint8_t returned_data;
  i2c_start_wait((TMP103G<<1)|I2C_WRITE);
  i2c_write(reg);
  i2c_rep_start((TMP103G<<1)|I2C_READ);
  returned_data = i2c_read(true); 
  i2c_stop(); 
  return returned_data;
}

int getTemperature(){
  // Reads TMP103 and returns temperature in celsius
  int temp = read_register(TEMPERATURE_REGISTER);           
  temp = constrain(temp, MIN_TEMPERATURE, MAX_TEMPERATURE);
  return temp;
}

// #######################################
// ##         GENERAL METHODS           ##
// #######################################

int doubleAnalogRead(uint8_t pin){
  int adcValue = analogRead(pin);
  adcValue = analogRead(pin);
  return adcValue;
}

void runCandlelight(){
  // Candlelight mode - needs a lot of work.., currently not good!
  runningCandlelight = 1;
  while (runningCandlelight == 1){
    checkBattery();
    checkCriticalTemperature();
    pwmPinWrite((4+random(2)-random(2)));
    delay(random(70)+random(50)+random(20));
  }
}

void checkBattery(){
  /*
  checkBattery() reads the battery voltage a few times:
  If it's low for a few times (to smooth readings
    e.g. bVreadings times in a row) it then drops state.
  If the state is 0, it checks for critical battery instead.
  If it's below critical battery, turn light off.
  */

  int bV = doubleAnalogRead(Pin_Batt);
  if (currentState == 0){
    // Lowest state already, monitor for critical battery
    if (bV < CRITICAL_BATTERY){
      bVreadings++;
    }
    else{
      bVreadings = 0;
    }
    if (bVreadings >= 10){
      // Battery is Critically low
      ErrorFlashBatteryCritical();
    }
  }
  else{
    // Current state is > 0, check for low battery
    if (bV < LOW_BATTERY){
      bVreadings++;
    }
    else{
      bVreadings = 0;
    }
    if (bVreadings>=10){
      // Battery is low, drop state by 1 and flash error message
      runningCandlelight = 0;
      ErrorFlashBatteryLow();
      drop_state();
    }
  }
}

// #######################################
// ##       ERROR HANDLING              ##
// #######################################

void checkCriticalTemperature(){
  // Check if temperature is Critical. If it is, turn off.
  int temp = getTemperature();
    if (temp>=CRITICAL_TEMPERATURE){
    ErrorFlashCriticalTemperature();
  }
}

// Method for flashing certain number of times
void flash(uint8_t flashes){
  uint8_t i = 0;
  rampFromValToOff(currentBrightness,2);
  digitalWrite(Pin_Enable,LOW);
  delay(200);
  pwmPinWrite(2);

  while (i<flashes){
    digitalWrite(Pin_Enable,HIGH);
    delay(233);
    digitalWrite(Pin_Enable,LOW);
    delay(300);
    i++;
  }
}

void ErrorFlashBatteryLow(){
  flash(3);
}

void ErrorFlashBatteryCritical(){
  // Critical Battery
  // Turns off LED completely, requires hard turn-off and on for safety.
  flash(5);
  currentState = 0;
  save_state();
  while (1){
    // loop forever
  }
}

void ErrorFlashCriticalTemperature(){
  // Critical Temperature
  // Turns off LED completely, requires hard turn-off and on for safety.
  flash(4);
  currentState = 0;
  save_state();
  while (1){
    // loop forever
  }
}

// #########################################
// ## METHODS FOR CHANGING LED BRIGHTNESS ##
// #########################################

void pwmPinWrite(uint16_t val){
  // 10 bit brightness value
  if (val <=0){
    digitalWrite(Pin_PWM,LOW);
  }
  if (val >= 1023){
    digitalWrite(Pin_PWM,HIGH);
  }
  TCCR2A &= ~(1<<COM2B0);
  TCCR2A |= (1<<COM2B1);
  OCR2B = val;
  currentBrightness = val;
}

void rampFromValToVal(int fval, int tval, int speed){
  // speed = 1 = fastest
  int i = fval;
  if (fval<tval){
    while (i < tval){
      i+=1;
      pwmPinWrite(i);
      delay(speed);
    }
  }
  else {
    while (i>tval){
      i-=1;
      pwmPinWrite(i);
      delay(speed);
    }
  }
  pwmPinWrite(tval);
}

void rampToVal(int pwm, int speed){
  // speed = 1 = fastest
  int i= 0;
  if (pwm<0){
    pwm = 0;  // handles cases like candlelight mode etc
  }
  digitalWrite(Pin_Enable,HIGH);
  while (i<(pwm-5)){
      i+=5;
      pwmPinWrite(i);
      delay(speed);
    }
  pwmPinWrite(pwm);
}

void rampFromValToOff(int pwm, int speed){
  // speed = 1 = fastest
  int i = pwm;
  while (i>0){
      i-=speed;
      pwmPinWrite(i);
      delay(speed);
    }
  digitalWrite(Pin_Enable,LOW);
}

void runLED(byte s){
  // run the LED
  int newBrightness = brightnessValues[s];

  // Initialize with lowest brightness
  pwmPinWrite(0);
  digitalWrite(Pin_Enable,HIGH);

  if (newBrightness == -1){
    runCandlelight();
  }

  // if current is greater than ~500mA, enable PID
  if (newBrightness > 70){
    tempPID.SetOutputLimits(PID_LOW_LIMIT,(double)newBrightness);
    Output = (double)newBrightness;
    tempPID.Initialize();
    runningPID = 1;
  }

  // If battery voltage is below 3.7V, do not run mode above ~3 Amps (514/1023)
  // Flash error message and drop one state to alert user

  int bV = doubleAnalogRead(Pin_Batt);
  if (bV < 265 && newBrightness > 514){
    drop_state();
    ErrorFlashBatteryLow();
  }

  // Else all is ok
  else{
    rampToVal(newBrightness,8);
  }
}


void handleModeGroups(){

  // Allows us to set some mode groups with X1 and X2 jumpers
  // Default unbriged = HIGH (input pullups)
  /*
  PB0 PB1
  X1  X2   Modes (current in mA)              Set Temp   Crit Temp (deg C)
  1   1    Low   Candle  250   1000   5000    60C        90C      (default mode)
  1   0    Low   50      250   1500           60C        90C
  0   1    Low   50      250   1000   4200    60C        90C
  0   0    Low   50      250   1000   5500    60C        90C        
  Increase the current at your own risk ;)...
  */

  int x1 = digitalRead(Pin_PB0);
  int x2 = digitalRead(Pin_PB1);

  if(x1 == HIGH && x2 == LOW){
    NUMSTATES = 4;
    brightnessValues[1] = 8;
    //brightnessValues[2] = 45;
    brightnessValues[3] = 275;
    if (currentState>=4){
      currentState = 0;
    }
  } 
  else if(x1 == LOW && x2 == HIGH){
    brightnessValues[1] = 8;
    brightnessValues[4] = 726;
  }
  else if(x1 == LOW && x2 == LOW){
    brightnessValues[1] = 8;
    brightnessValues[4] = 938;
  }
}

// #######################################
// ##           INITILIZATION           ##
// #######################################

void setup(){
  analogReference(DEFAULT);
  pinMode(Pin_LedDbg,OUTPUT);
  pinMode(Pin_OTCD,INPUT);
  
  // First up let's read ADC Value on OTC
  int otc_val = doubleAnalogRead(Pin_OTCA);

  // Next restore state and take action based on OTC reading
  // Long Press not implemented yet.. TODO
  restore_state();
  if(otc_val>CAP_SHORT){
    next_state();
  }
  else{
    // Long press, or reset to the first mode
    if (!memory){
      currentState = 0;
    }
  }

  // Now Setup Pins
  pinMode(Pin_PWM,OUTPUT);
  pinMode(Pin_Enable,OUTPUT);
  pinMode(Pin_OTCD, OUTPUT);
  pinMode(Pin_PB0,INPUT_PULLUP);
  pinMode(Pin_PB1,INPUT_PULLUP);
  pinMode(Pin_Batt,INPUT);
  
  digitalWrite(Pin_OTCD,HIGH);
  digitalWrite(Pin_Enable,LOW);
  digitalWrite(Pin_PWM,LOW);

  handleModeGroups();

  save_state();

  // 7.8kHz 10bit PWM setup
  TCCR2B = (TCCR2B & 0b11111000) | 0x01;
  TCCR2B &= ~(1 << WGM23);  // Timer B clear bit
  TCCR2B |=  (1 << WGM22);  // set bit
  TCCR2A |= (1 << WGM21);   //  Timer A set bit
  TCCR2A |= (1 << WGM20);   //  set bit

  // If I2C does not initialize, flash error message
  if (!i2c_init()) ErrorFlashCriticalTemperature();

  // Initialize PID
  Input = getTemperature();   // Initialize starting value to be starting Temperature instead of 0
  Setpoint = SET_TEMPERATURE; // Set our PID to be the desired Set Temperature
  tempPID.SetMode(AUTOMATIC);

}

// #######################################
// ##      MAIN APPLICATION LOOP        ##
// #######################################

void loop(){

  // Run LED depending on state
  if (currentState!=olderState){
    olderState = currentState; 
    runLED(currentState);   
  }

  // Now compute PID or run regular error handling

  if (runningPID == 1){
    checkCriticalTemperature();
    checkBattery();
    delay(25);
    Input = getTemperature();
    tempPID.Compute();
    pwmPinWrite(Output);
  }

  else{
    // not running PID; light is on lower power levels
    checkCriticalTemperature();
    delay(25);
    checkBattery();
    // Delay a little since we don't need to keep doing readings
    delay(425); 
  }
}
