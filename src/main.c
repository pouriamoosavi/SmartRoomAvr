#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "DS3232_lib.h"
#include "i2c_lib.h"
#include "liquid_crystal_i2c_lib.h"
#include "Serial_lib.h"

const char* password = "1234";
const char* setTimeCommand = "set time";
const char* setLampCommand = "set lamp";
const char* openCurtainCommand = "open curtain";
const char* closeCurtainCommand = "close curtain";
const char* helpCommand = "help";

volatile char command[29]; //longest command is set time "set time 12:34:56 06/30/2023" + \0
volatile uint8_t commandIndex = 0;
volatile bool commandReady = false;
volatile bool loggedIn = false;
volatile bool reRenderLcd = false;
volatile uint8_t secondsPastLastCommand = 0;

volatile bool curtain = true;
volatile uint8_t lamp = 0;

volatile DateTime_t t;

void println(char* str) {
  serial_send_string(str);
  serial_send_char('\r');
}

#if SERIAL_INTERRUPT == 1
ISR(USART_RXC_vect) {
  if(commandIndex == 29) {
    println("\rYour input is too long! Please try again:");
    commandIndex = 0;
  }

	char c = UDR;
	// UDR = c; // wrong way to mirror. some characters will be lost due to not wait for TXC to clear;
  serial_send_char(c);
  secondsPastLastCommand = 0;
  
	if(c == '\r'){
    command[commandIndex] = '\0';
    commandIndex = 0;
    commandReady = true;
	} else {
    command[commandIndex] = c;
    commandIndex++;
  }
}
#endif

ISR(INT2_vect) {
  reRenderLcd = true;
  if(loggedIn) {
    secondsPastLastCommand++;
    if(secondsPastLastCommand == 60) {
      loggedIn = false;
      println("Logged out. Enter password to login:");
    }
  }
}

void PWM_init() {
	TCCR0 = (1<<WGM00) | (1<<COM01) | (1 << CS02); 
	DDRB |= (1<<PB3); 
}

void printOnLcd(LiquidCrystalDevice_t lcd1) {
	lq_setCursor(&lcd1, 0, 12);
  char* curtainStr = (char*)"on";
  if(!curtain) {
    curtainStr = (char*)"off";
  }
	lq_print(&lcd1, curtainStr);
	
  char lampStr[4] = "";
  sprintf(lampStr, "%d", lamp);
  // println(lampStr);
  // lampStr[0] = lamp[0];
  // lampStr[1] = lamp[1];
  // lampStr[3] = '%';
  // lampStr[4] = '\0';
  // println(lampStr);
	lq_setCursor(&lcd1, 1, 12);
	lq_print(&lcd1, lampStr);
  if(lamp < 100) {
    lq_setCursor(&lcd1, 1, 14);
	  lq_print(&lcd1, " ");
  }
  lq_setCursor(&lcd1, 1, 15);
	lq_print(&lcd1, "%");
	
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

void setTime() {
  sscanf((char*)command, "set time %" SCNu8 ":%" SCNu8 ":%" SCNu8 " %" SCNu8 "/%" SCNu8 "/%" SCNu16, &t.Hour, &t.Minute, &t.Second, &t.Month, &t.Date, &t.Year);
	RTC_Set(t);
}

void setLamp() {
  sscanf((char*)command, "set lamp %" SCNu8, &lamp);
  uint8_t duty = 255 * lamp / 100;
  OCR0=duty;
}

void openCurtain() {
  int period = 50;
  for(short i=0; i< 9; i++) {
    PORTC = 0x04;
    _delay_ms(period);
    PORTC = 0x02;
    _delay_ms(period);
    PORTC = 0x08;
    _delay_ms(period);
    PORTC = 0x01;
    _delay_ms(period);
  }
  PORTC = 0x04;
  _delay_ms(period);
  PORTC = 0x02;

  curtain = true;
}

void closeCurtain() {
  int period = 50;
  for(short i=0; i< 9; i++) {
    PORTC = 0x01;
    _delay_ms(period);
    PORTC = 0x08;
    _delay_ms(period);
    PORTC = 0x02;
    _delay_ms(period);
    PORTC = 0x04;
    _delay_ms(period);
  }
  PORTC = 0x01;
  _delay_ms(period);
  PORTC = 0x08;
  _delay_ms(period);
  PORTC = 0x02;

  curtain = false;
}

void printHelp() {
  println("\
List of commands:\r\
- set time HH:mm:ss MM/DD/YYYY\r\
- set lamp dd\r\
- open curtain\r\
- close curtain\r\
If logout, just enter the password, hit enter and then enter your command.");
}

void runCommand() {
  commandReady = false;
  if(!loggedIn && strncmp((char*)command, password, 4) == 0) {
    loggedIn = true;
    println("Logged in! Enter your command or enter \"help\" to show the list of commands:");
    return;
  }

  if(!loggedIn) {
    println("Wrong password. Try again:");
    return;
  }

  if(strcasestr((char*)command, setTimeCommand) != NULL) {
    setTime();
    reRenderLcd = true;
    println("Done!");
  } else if(strcasestr((char*)command, setLampCommand) != NULL) {
    setLamp();
    reRenderLcd = true;
    println("Done!");
  } else if(strcasestr((char*)command, openCurtainCommand) != NULL) {
    openCurtain();
    reRenderLcd = true;
    println("Done!");
  } else if(strcasestr((char*)command, closeCurtainCommand) != NULL) {
    closeCurtain();
    reRenderLcd = true;
    println("Done!");
  } else if(strcasestr((char*)command, helpCommand) != NULL) {
    printHelp();
  } else {
    println("Unknown input. Write help for list of commands.");
  }
}

void initTime() {
	t.Second = 50;
	t.Minute = 10;
	t.Hour = 21;
	t.Date = 26;
	t.Month = (Month_t)2;
	t.Year = 23;
	RTC_Set(t);

  RTC_SetSquareWave(RTC_SQWAVE_1_HZ);
  // RTC_AlarmSet(Alarm1_Match_Seconds, 0, 0, 0, 2);
  // RTC_AlarmInterrupt(Alarm_1, 1);

  DDRB &= ~(1 << PB2);
	PORTB |= (1 << PB2);
	
	GICR = 1<<INT2;
	MCUCR = 1<<ISC01 | 1<<ISC00;
}

int main () {
  //both DS3232 and liquid crystal use i2c,
	//so we init i2c BEFORE USING THEM
	i2c_master_init(I2C_SCL_FREQUENCY_400);

	//create a liquid crystal:
	LiquidCrystalDevice_t lcd1 = lq_init(0b0100111, 16, 2, LCD_5x8DOTS);
	
	//create a time struct for DS3232
  initTime();

  printOnLcd(lcd1);

	//init serial
	serial_init();
	println("Welcome to the program!\rEnter password to login:"); //look at how \r works
	
	PWM_init();

	sei();

  while(1) {
    if(commandReady) {
      runCommand();
    }
    if(reRenderLcd) {
      reRenderLcd = false;
      printOnLcd(lcd1);
    }
  }
}