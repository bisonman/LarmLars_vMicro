/****************************************************************************
 * Project : LarmLars
 * Date : 2016-11-19
 */

#include <TimeLib.h>
#include <EEPROM.h>
#include <limits.h>
#include <RC5.h>

using namespace std;

#define TIMEOUT_STEPS  40       // Number of seconds for each step (1-9) 0 no timeout

//#####  I/O pins
int IR_PIN = 5;
int GREEN_LED_PIN = 6;
int RED_LED_PIN = 7;
int INTERRUPT_PIN = 3;
int RELAY_PIN = 4;
int INTERNAL_LED_PIN = 13;

RC5 rc5(5);

int maxTime = 1;					// Ställer in Larmtimeout * TIMEOuT_STEPS i antal sekunder
int EEPROM_AdrMaxTime = 0;          // Adress i EEPROM där "maxTime" lagras
int EEPROM_AdrPowerOnStatus = 2;    // Adress i EEPROM där flagga som sätter auto power vid boot

//#####  Status flags
boolean FlagPowerOn = false;
boolean FlagFlashRedLED = false;
boolean FlagSwitch = false;
//## boolean switchStatus = false;
boolean FlagGreenLed = false;
boolean FlagRedLed = false;
boolean FlagRelay = false;
boolean FlagLarm = false;
//## boolean FlagTimeOverflow = false;

int Second = 0;
int oldSecond = 0;
unsigned char oldToggle = 0;		// Används vid fjärrkontroll avläsning

unsigned long TimeLarm = 0;         // avläsning av millis() vid larm
unsigned long TimeLarmTimeout = 0;  // Hur lång tid reläet skall vara aktiverat vid larm


//**********************************************************
void
switchInterrupt()
{
	FlagSwitch = true;
  
	if (digitalRead(INTERRUPT_PIN) == 0) {
		if (FlagPowerOn == true) {
			FlagLarm = 1;
			TimeLarm = millis();
			TimeLarmTimeout = (maxTime * TIMEOUT_STEPS);
			FlagRelay = true;
			FlagRedLed = true;
			FlagFlashRedLED = false;      // Endast efter timeout
		}
	}
}

//**********************************************************
void
setup()
{
	FlagLarm = 0;
	TimeLarm = 0;
  
	maxTime = EEPROM.read(EEPROM_AdrMaxTime);

	if (!(maxTime >= 0 && maxTime <= 9)) {
		maxTime = 2;
		EEPROM.write(EEPROM_AdrMaxTime, maxTime);      // Write default value
	}
	FlagPowerOn = EEPROM.read(EEPROM_AdrPowerOnStatus);
  
	Serial.begin(115200);
	pinMode(INTERRUPT_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), switchInterrupt, FALLING);

	pinMode(GREEN_LED_PIN, OUTPUT);
	pinMode(RED_LED_PIN, OUTPUT);
	pinMode(RELAY_PIN, OUTPUT);
	pinMode(INTERNAL_LED_PIN, OUTPUT);

	digitalWrite(GREEN_LED_PIN, HIGH);
	digitalWrite(RED_LED_PIN, HIGH);
	digitalWrite(RELAY_PIN, LOW);

	FlagFlashRedLED = false;
	FlagRedLed = false;
	Serial.println("ATOK");
}

//**********************************************************
void
showInformation()
{
	int i;
	int numberOfFlashes = maxTime;
	Serial.print("FlagPowerOn="); Serial.println(FlagPowerOn);
	Serial.print("EEPROM FlagPowerOn="); Serial.println(EEPROM.read(EEPROM_AdrPowerOnStatus));
	Serial.print("FlagLarm="); Serial.println(FlagLarm);
	Serial.print("maxtime="); Serial.println(maxTime);
	Serial.print("FlagFlashRedLED="); Serial.println(FlagFlashRedLED);
	Serial.print("FlagRedLed="); Serial.println(FlagRedLed);
	delay(2000);
  
	if (maxTime == 0) {
		numberOfFlashes = 10;
	}
	for (i = 0; i < numberOfFlashes; i++) {
		delay(200);
		digitalWrite(GREEN_LED_PIN, LOW);
		delay(200);
		digitalWrite(GREEN_LED_PIN, HIGH);
	}
	delay(2000);
}

//**********************************************************
void
flashGreenLED(int n)
{
	int v = digitalRead(GREEN_LED_PIN);

	digitalWrite(GREEN_LED_PIN, HIGH);
	delay(200);

	for (int i=0; i<n; i++) {
		digitalWrite(GREEN_LED_PIN, LOW);
		delay(100);
		digitalWrite(GREEN_LED_PIN, HIGH);
		delay(100);
	}
	digitalWrite(GREEN_LED_PIN, v);
}

//**********************************************************
void
flashRedLED(int n)
{
	int v = digitalRead(RED_LED_PIN);

	digitalWrite(RED_LED_PIN, HIGH);
	delay(200);
	
	for (int i=0; i<n; i++) {
		digitalWrite(RED_LED_PIN, LOW);
		delay(100);
		digitalWrite(RED_LED_PIN, HIGH);
		delay(100);
	}	
	digitalWrite(RED_LED_PIN, v);
}
//**********************************************************
void
handleRemoteController()
{
	unsigned char toggle;
	unsigned char address;
	unsigned char command = 0;

	if (rc5.read(&toggle, &address, &command)) {
		Serial.print("a: "); Serial.print(address);
		Serial.print(" c: "); Serial.print(command);
		Serial.print(" t: "); Serial.println(toggle);

		if (toggle != oldToggle) {
			
			switch ((int) command) {
				case 0:
				case 1:
		        case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
					flashGreenLED(2);
					maxTime = command;
					EEPROM.write(EEPROM_AdrMaxTime, maxTime);
					break;

				case 12:
					flashGreenLED(3);
					if (FlagPowerOn) {
						FlagPowerOn = false;
						FlagGreenLed = false;
						Serial.println("OFF");
					}
					else {
						FlagPowerOn = true;
						FlagGreenLed = true;
						Serial.println("ON");

						if (!digitalRead(INTERRUPT_PIN)) {
							FlagRedLed = true;
							FlagFlashRedLED = true;
						}
					}
					break;

				case 15:
					flashGreenLED(4);
					showInformation();
					break;
			
				case 43:
					flashGreenLED(5);
					if (EEPROM.read(EEPROM_AdrPowerOnStatus)) {
						EEPROM.write(EEPROM_AdrPowerOnStatus, 0);
						flashRedLED(2);
					}
					else {
						EEPROM.write(EEPROM_AdrPowerOnStatus, 1);
						flashRedLED(2);
					}
					break;
			}
		}
		oldToggle = toggle;
	}
}

//**********************************************************
void
handleSecondChangeAction()
{
	if (FlagPowerOn == true) {
		if (FlagLarm == true) {
			if (maxTime > 0) {					// Om maxTime = 0 ingen timeout
				Serial.println(TimeLarmTimeout);
		  
				if (TimeLarmTimeout > 0L) {
					TimeLarmTimeout--;
			
					if (TimeLarmTimeout == 0L) {
						FlagRelay = false;			// Tid att stänga av larm signalen
						FlagRedLed = true;
						FlagFlashRedLED = true;
						FlagLarm = false;	
					}
				}
			}
		}
		FlagGreenLed = true;
	}
	else {
		FlagGreenLed = false;
	}
  
	if (FlagFlashRedLED == true) {
		if (oldSecond & 1) {
			digitalWrite(RED_LED_PIN, LOW);
		}
		 else {
			digitalWrite(RED_LED_PIN, HIGH);
		}
	}
	else {
		if (FlagRedLed == true) {
			digitalWrite(RED_LED_PIN, HIGH);
		}
		else {
			digitalWrite(RED_LED_PIN, LOW);
		}
	}
}

//**********************************************************
void
loop()
{
	handleRemoteController();

	if (digitalRead(INTERRUPT_PIN)) {			// Används vid installations test
		digitalWrite(INTERNAL_LED_PIN, HIGH);
	}
	else {
		digitalWrite(INTERNAL_LED_PIN, LOW);
	}
  
	Second = second();
  
	if (Second != oldSecond) {
		handleSecondChangeAction();
		oldSecond = Second;
	}
	if (FlagGreenLed == true) {
		digitalWrite(GREEN_LED_PIN, LOW);
	}
	else {
		digitalWrite(GREEN_LED_PIN, HIGH);
	}
	if (FlagRelay == true) {
		digitalWrite(RELAY_PIN, HIGH);
	}
	else {
		digitalWrite(RELAY_PIN, LOW);
	}
}
