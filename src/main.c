#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "DS3232_lib.h"
#include "i2c_lib.h"
#include "liquid_crystal_i2c_lib.h"
#include "Serial_lib.h"

#define PWM_PIN PB1
#define PWM_FREQ 1000 // Frequency in Hz
#define PWM_RESOLUTION 255 // 8-bit resolution

volatile char command[38];
volatile uint8_t commandIndex = 0;
volatile bool runCommand = false;

volatile bool curtain = false;
volatile char lamp[2];

#if SERIAL_INTERRUPT == 1
ISR(USART_RXC_vect) {
	char c = UDR;
	UDR = c;
  
  command[commandIndex] = c;
  commandIndex++;

	if(c == '\r'){
    command[commandIndex] = '\0';
    commandIndex = 0;
    runCommand = true;
	}
}
#endif

void PWM_init() {
	/*set fast PWM mode with non-inverted output*/
	TCCR0 = (1<<WGM00) | (1<<WGM01) | (1<<COM01) | (1<<CS00);
	DDRB|=(1<<PB3);  /*set OC0 pin as output*/
}

void printOnLcd(LiquidCrystalDevice_t lcd1) {
	lq_setCursor(&lcd1, 0, 13);
  char* curtainStr = (char*)"on";
  if(!curtain) {
    curtainStr = (char*)"off";
  }
	lq_print(&lcd1, curtainStr);
	
  char lampStr[4];
  lampStr[0] = lamp[0];
  lampStr[1] = lamp[1];
  lampStr[2] = '%';
  lampStr[3] = '\0';
	lq_setCursor(&lcd1, 1, 13);
	lq_print(&lcd1, lampStr);
	
	DateTime_t t = RTC_Get();
	if (RTC_Status() == RTC_OK) {
		//print time on lcd
		lq_setCursor(&lcd1, 0, 0);
		char timeArr[10];
		sprintf(timeArr, "%02d:%02d:%02d", t.Hour, t.Minute, t.Second);
		lq_print(&lcd1, timeArr);
		
		//print date on lcd
		lq_setCursor(&lcd1, 1, 0);
		char dateArr[10];
		sprintf(dateArr, "%02d/%02d/%02d", t.Year, t.Month, t.Date);
		lq_print(&lcd1, dateArr);
	}
}

int main () {
  //both DS3232 and liquid crystal use i2c,
	//so we init i2c BEFORE USING THEM
	i2c_master_init(I2C_SCL_FREQUENCY_400);

	//create a liquid crystal:
	LiquidCrystalDevice_t lcd1 = lq_init(0b0100111, 16, 2, LCD_5x8DOTS);
	
	//create a time struct for DS3232
	DateTime_t t;
	t.Second = 57;
	t.Minute = 59;
	t.Hour = 23;
	t.Date = 29;
	t.Month = (Month_t)4;
	t.Year = 2025;
	RTC_Set(t);

  printOnLcd(lcd1);

	//init serial
	serial_init();
	serial_send_string("Welcome to the program!\r"); //look at how \r works
	
	//enable interrupts.
	//Who uses interrupts? Serial!
	sei();
	
	unsigned char duty;
	
	PWM_init();
	while (1) {
		for(duty=0; duty<255; duty++)
		{	
			OCR0=duty;  /*increase the LED light intensity*/
			_delay_ms(8);
		}
		for(duty=255; duty>1; duty--)
		{
			OCR0=duty;  /*decrease the LED light intensity*/
			_delay_ms(8);
		}
	}
}