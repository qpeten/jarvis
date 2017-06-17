/**
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2015 Sensnology AB
* Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
*******************************
*
* DESCRIPTION
* The ArduinoGateway prints data received from sensors on the serial link.
* The gateway accepts input on seral which will be sent out on radio network.
*
* The GW code is designed for Arduino Nano 328p / 16MHz
*
* Wire connections (OPTIONAL):
* - Inclusion button should be connected between digital pin 3 and GND
* - RX/TX/ERR leds need to be connected between +5V (anode) and digital pin 6/5/4 with resistor 270-330R in a series
*
* LEDs (OPTIONAL):
* - To use the feature, uncomment any of the MY_DEFAULT_xx_LED_PINs
* - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
* - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
* - ERR (red) - fast blink on error during transmission error or recieve crc error
*
*/

/**
 * This GW has also controls a light.
 */

// Enable debug prints to serial monitor
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level.
#define MY_RF24_PA_LEVEL RF24_PA_LOW

// Define a lower baud rate for Arduino's running on 8 MHz (Arduino Pro Mini 3.3V & SenseBender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif

#define MY_NODE_ID 1

#include <MySensors.h>

#define PIN_LIGHT_RELAY 4
#define PIN_BOILER_PARENTS_RELAY 5
#define PIN_BOILER_AYMERIC_RELAY 6
#define CHILD_SENSOR_ID_MOTION_ON 1
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay
#define PIN_SWITCH_INPUT 2 // Pin used to detect the switch state
#define PIN_MOTION_SENSOR 6
#define SWITCH_CHANGE_DEBOUNCE_MILLIS 275
#define SWITCH_MAX_TIME_BETWEEN_KNOCKS 1000
#define LIGHT_ON_LONG_TIME 3600000
#define LIGHT_ON_SHORT_TIME 60000

typedef enum lightStatus {
  OFF=0,
  ON_SHORT=1,
  ON_LONG=2
} lightStatus;

//bool motionSensorOn = true;
bool lastSwitchState;
short nbrKnocks=1;
unsigned long lastSwitchChange=0;
unsigned long lastLightOn=0;
lightStatus light = OFF;

MyMessage msg(PIN_LIGHT_RELAY+200, V_STATUS);

void before() {
  pinMode(PIN_LIGHT_RELAY, OUTPUT);
  pinMode(PIN_BOILER_PARENTS_RELAY, OUTPUT);
  pinMode(PIN_BOILER_AYMERIC_RELAY, OUTPUT);
  digitalWrite(PIN_LIGHT_RELAY, RELAY_OFF);
	digitalWrite(PIN_BOILER_PARENTS_RELAY, loadState(PIN_BOILER_PARENTS_RELAY) ? RELAY_ON : RELAY_OFF);
  digitalWrite(PIN_BOILER_AYMERIC_RELAY, loadState(PIN_BOILER_AYMERIC_RELAY) ? RELAY_ON : RELAY_OFF);
  
  pinMode(PIN_SWITCH_INPUT, INPUT_PULLUP);
  lastSwitchState = digitalRead(PIN_SWITCH_INPUT);
}

void setup()
{
  
}

void presentation()
{
  sendSketchInfo("GarageLight", "0.3");
  present(getChildSensorIDForGW(PIN_LIGHT_RELAY), S_BINARY, "Lumière Garage");
	present(getChildSensorIDForGW(PIN_BOILER_PARENTS_RELAY), S_BINARY, "Boiler Parents");
	present(getChildSensorIDForGW(PIN_BOILER_AYMERIC_RELAY), S_BINARY, "Boiler Aymeric");
//  present(getChildSensorIDForGW(CHILD_SENSOR_ID_MOTION_ON), S_BINARY, "Is it daylight");
}

void loop()
{
  manageSwitchToggleOnly();
  manageLightTimer();
}

void manageLightTimer(){
  if (light == ON_SHORT && millis() - lastLightOn > LIGHT_ON_SHORT_TIME) {
    turnLightOff();
  }
  else if (light == ON_LONG && millis() - lastLightOn > LIGHT_ON_LONG_TIME) {
    turnLightOff();
  }
}

bool hasSwitchChanged() {
  if (millis() - lastSwitchChange > SWITCH_CHANGE_DEBOUNCE_MILLIS &&
      lastSwitchState != digitalRead(PIN_SWITCH_INPUT)) {
    lastSwitchState = !lastSwitchState;
		lastSwitchChange = millis();
    return true;
  }
  return false;
}

void manageSwitchToggleOnly() {
  if (hasSwitchChanged()) {
    toggleLight();
  }
}

void manageSwitch() {
  bool switchTriggered = hasSwitchChanged();
  if (switchTriggered) {
    manageKnocks();
    if (nbrKnocks == 1 &&
        millis() - lastLightOn > SWITCH_CHANGE_DEBOUNCE_MILLIS) { //Avoids changing the lightstate if motion was detected just before the switch was pressed
      toggleLight();
    }
    else if (nbrKnocks == 2) {
      nbrKnocks = 1;
      turnLightOn(false);
    }
  }  
}

void manageKnocks() {
  if (nbrKnocks == 0) {
    nbrKnocks = 1;
  }
  else if (millis() - lastSwitchChange < SWITCH_MAX_TIME_BETWEEN_KNOCKS*nbrKnocks) {
    nbrKnocks++;
  }
  else {
    nbrKnocks = 1;
  }
}

void toggleLight() {
	if (digitalRead(PIN_LIGHT_RELAY) == RELAY_ON) {
		turnLightOff();
	}
	else {
		turnLightOn(true);
	}
}

void changeLightState(bool newState) {
  bool toWrite;
  switch (newState) {
    case true:  toWrite = RELAY_ON; break;
    case false: toWrite = RELAY_OFF;break;
  }
  digitalWrite(PIN_LIGHT_RELAY, toWrite);
  //saveState(PIN_LIGHT_RELAY, toWrite);
  send(msg.set(newState));
}

//onShort should be set to false if the light is supposed to stay on for a long time
void turnLightOn(bool onShort) {
  if (digitalRead(PIN_LIGHT_RELAY) == RELAY_OFF) { //Avoid sending repeated messages when detecting motion; but still update lastLightOn
    changeLightState(true);
  }
  if (onShort && light != ON_LONG) { //Avoid interrupting ON_LONG when sensing motion
    light = ON_SHORT;
  }
  else if (!onShort) {
    light = ON_LONG;
  }
  lastLightOn = millis();
}

void turnLightOff() {
  if (digitalRead(PIN_LIGHT_RELAY) == RELAY_ON) {
    changeLightState(false);
  }
  light = OFF;
}

uint8_t getChildSensorIDForGW(uint8_t sensorID) {
  return 200+sensorID;
}

void setRelay(uint8_t pin, bool value) {
	digitalWrite(pin, value ? RELAY_ON : RELAY_OFF);
	saveState(pin, value);
}

void receive(const MyMessage &message)
{
  if (message.type==V_STATUS) {
    if (message.sensor == getChildSensorIDForGW(PIN_LIGHT_RELAY)) {
      if (message.getBool()) {
        turnLightOn(false);
      }
      else {
        turnLightOff();
      }
    }
		else if (message.sensor == getChildSensorIDForGW(PIN_BOILER_PARENTS_RELAY)) {
			setRelay(PIN_BOILER_PARENTS_RELAY, message.getBool());
		}
		else if (message.sensor == getChildSensorIDForGW(PIN_BOILER_AYMERIC_RELAY)) {
			setRelay(PIN_BOILER_AYMERIC_RELAY, message.getBool());
		}
    else {
      Serial.print("Error: Received wrong sensor number.");
    }
  }
}
