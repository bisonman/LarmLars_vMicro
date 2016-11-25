/****************************************************************************
 * Project : LarmLars
 * Date : 2016-11-19
 */


#include <TimeLib.h>
#include <EEPROM.h>
#include <limits.h>
#include <RC5.h>

// using namespace std;

#define TIMEOUT_STEPS  20       // Number of seconds for each step (1-9) 0 no timeout

boolean switchFlag = false;
boolean switchStatus = false;
int IR_PIN = 5;
int GREEN_LED_PIN = 6;
int RED_LED_PIN = 7;
int INTERRUPT_PIN = 3;
int RELAY_PIN = 4;
int INTERNAL_LED_PIN = 13;
unsigned long t0;

RC5 rc5(5);

int maxTime = 1;
int maxTimeAdrEEPROM = 0;
int powerOnStatusEEPROM = 2;

boolean powerOn = false;
boolean flashRedLED = false;

boolean green_led_status = false;
boolean red_led_status = false;
boolean relay_status = false;

int Second = 0;
int oldSecond = 0;
// unsigned long currentTime = 0;
// unsigned long oldCurrentTime = 0;
unsigned char oldToggle = 0;

boolean larmFlag = false;
boolean timeOverflow = false;
unsigned long larmTime = 0;
unsigned long larmTimeout = 0;

//**********************************************************
void
switchInterrupt()
{
  switchFlag = true;

  if (digitalRead(INTERRUPT_PIN)) {
    switchStatus = true;
  }
  else {
    switchStatus = false;
    
    if (powerOn == true) {
      larmFlag = 1;
      larmTime = millis();
      larmTimeout = (maxTime * TIMEOUT_STEPS);
      relay_status = true;
      red_led_status = true;
      flashRedLED = false;      // Endast efter timeout
    }
  }
}

//**********************************************************
void
setup()
{
  larmFlag = 0;
  larmTime = 0;
  
  maxTime = EEPROM.read(maxTimeAdrEEPROM);

  if (maxTime < 0 || maxTime > 9) {
    maxTime = 2;
    EEPROM.write(maxTimeAdrEEPROM, maxTime);      // Write default value
  }
  powerOn = EEPROM.read(powerOnStatusEEPROM);
  
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

  flashRedLED = false;
  red_led_status = false;
  Serial.println("ATOK");
}

//**********************************************************
void
visaInformation()
{
  int i;
  int numberOfFlashes = maxTime;
  Serial.print("powerOn="); Serial.println(powerOn);
  Serial.print("EEPROM powerOn="); Serial.println(EEPROM.read(powerOnStatusEEPROM));
  Serial.print("larmFlag="); Serial.println(larmFlag);
  Serial.print("maxtime="); Serial.println(maxTime);
  Serial.print("flashRedLED="); Serial.println(flashRedLED);
  Serial.print("red_led_status="); Serial.println(red_led_status);
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
flashGreenLED()
{
  int v = digitalRead(GREEN_LED_PIN);

  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(100);
  digitalWrite(GREEN_LED_PIN, LOW);
  delay(100);
  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(100);
  digitalWrite(GREEN_LED_PIN, LOW);
  delay(100);
  digitalWrite(GREEN_LED_PIN, v);
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
    Serial.print(" ot: "); Serial.print(oldToggle);
    Serial.print(" t: "); Serial.println(toggle);
    flashGreenLED();

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
          maxTime = command;
          EEPROM.write(maxTimeAdrEEPROM, maxTime);
          break;

        case 12:
          if (powerOn) {
            powerOn = false;
            green_led_status = false;
            Serial.println("OFF");
          }
          else {
            powerOn = true;
            green_led_status = true;
            Serial.println("ON");

            if (!digitalRead(INTERRUPT_PIN)) {
              red_led_status = true;
              flashRedLED = true;
            }
          }
          break;

		case 15:
			visaInformation();
			break;
			
		case 43:
			if (EEPROM.read(powerOnStatusEEPROM)) {
				EEPROM.write(powerOnStatusEEPROM, 0);
			}
			else {
				EEPROM.write(powerOnStatusEEPROM, 1);
			}
			break;
      }
    }
    oldToggle = toggle;
  }
}

//**********************************************************
void
handleSecondAction()
{
  if (powerOn == true) {
    if (larmFlag == true) {
      Serial.println(larmTimeout);
      if (larmTimeout > 0L) {
        larmTimeout--;
        if (larmTimeout == 0L) {
          relay_status = false;
          red_led_status = true;
          flashRedLED = true;
          larmFlag = false;
        }
      }
    }
    green_led_status = true;
  }
  else {
    green_led_status = false;
  }
  
  if (red_led_status == true) {
    if (flashRedLED == true) {
      if (oldSecond & 1) {
        digitalWrite(RED_LED_PIN, LOW);
      }
      else {
        digitalWrite(RED_LED_PIN, HIGH);
      }
    }
    else {
       digitalWrite(RED_LED_PIN, LOW);
    }
  }
  else {
    digitalWrite(RED_LED_PIN, HIGH);
  }
}

//**********************************************************
void
loop()
{
  handleRemoteController();

  if (digitalRead(INTERRUPT_PIN)) {
    digitalWrite(INTERNAL_LED_PIN, HIGH);
  }
  else {
    digitalWrite(INTERNAL_LED_PIN, LOW);
  }
  Second = second();
  
  if (Second != oldSecond) {
    handleSecondAction();
    oldSecond = Second;
  }
  if (green_led_status == true) {
    digitalWrite(GREEN_LED_PIN, LOW);
  }
  else {
    digitalWrite(GREEN_LED_PIN, HIGH);
  }
  if (relay_status == true) {
    digitalWrite(RELAY_PIN, HIGH);
  }
  else {
    digitalWrite(RELAY_PIN, LOW);
  }
}
