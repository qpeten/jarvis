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
 * REVISION HISTORY
 * Version 1.0 - Henrik Ekblad
 *
 * DESCRIPTION
 * Motion Sensor example using HC-SR501
 * http://www.mysensors.org/build/motion
 *
 */

// Enable debug prints
// #define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <MySensors.h>

typedef enum lightStatus {
  OFF=0,
  ON_SHORT=1,
  ON_LONG=2
} lightStatus;

bool lastSwitchState;
short nbrKnocks=1;
unsigned long lastSwitchChange=0;
unsigned long lastLightOn=0;
lightStatus light = OFF;

#define DIGITAL_INPUT_SENSOR 3   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define PIN_LIGHT_RELAY 4
#define PIN_SWITCH_INPUT 5
#define PIN_MOTION_SENSOR 6
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay
#define SWITCH_CHANGE_DEBOUNCE_MILLIS 100
#define SWITCH_MAX_TIME_BETWEEN_KNOCKS 800
#define LIGHT_ON_LONG_TIME 3600000
#define LIGHT_ON_SHORT_TIME 60000

// Initialize motion message
MyMessage msg(PIN_LIGHT_RELAY, V_STATUS);

void setup()
{
  pinMode(PIN_LIGHT_RELAY, OUTPUT);
  //@@@TODO : restore old setting
  pinMode(PIN_SWITCH_INPUT, INPUT);
  pinMode(PIN_MOTION_SENSOR, INPUT);
}

void presentation()
{
  sendSketchInfo("LightViaMotionAndSwitch", "0.1");
  present(PIN_LIGHT_RELAY, S_BINARY);
}

void loop()
{
  manageMotion();
  manageSwitch();
  manageLightTimer();
}

void manageMotion() {
  bool motion = digitalRead(PIN_MOTION_SENSOR) == HIGH;
  turnLightOn(true);
}

void manageSwitch() {
  bool switchTriggered = hasSwitchChanged();
  if (switchTriggered) {
    manageKnocks();
    if (nbrKnocks == 1 &&
        millis() - lastLightOn > SWITCH_CHANGE_DEBOUNCE_MILLIS) {
      toggleGarageLight();
    }
    else if (nbrKnocks == 2) {
      nbrKnocks = 0;
      turnLightOn(false);
    }
  }  
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
    return true;
  }
  return false;
}

void manageKnocks() {
  if (nbrKnocks == 0) {
    nbrKnocks = 1;
    lastSwitchChange = millis();
  }
  else if (millis() - lastSwitchChange < SWITCH_MAX_TIME_BETWEEN_KNOCKS*nbrKnocks) {
    nbrKnocks++;
  }
  else {
    nbrKnocks = 0;
  }
}

void toggleGarageLight() {
  if (digitalRead(PIN_LIGHT_RELAY)) {
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
  saveState(PIN_LIGHT_RELAY, toWrite);
  send(msg.set(newState));
}

//onShort should be set to false if the light is supposed to stay on for a long time
void turnLightOn(bool onShort) {
  changeLightState(true);
  switch (onShort) {
    case true:  light = ON_SHORT;  break;
    case false: light = ON_LONG;   break;
  }
  lastLightOn = millis();
}

void turnLightOff() {
  changeLightState(false);
  light = OFF;
}
