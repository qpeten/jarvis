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
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

#define MY_NODE_ID 2

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enable repeater functionality for this node
//#define MY_REPEATER_FEATURE

#include <MySensors.h>

typedef enum fanSpeed {
  MIN = 1,
  NORMAL = 2,
  MAX = 3,
  AUTOMATIC = 0
} fanSpeed;

#define FAN_FIRST_PIN 4
#define RELAY_ON 0  // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 // GPIO value to write to turn off attached relay
#define DEFAULT_SPEED MIN


void before()
{
  pinMode(FAN_FIRST_PIN, OUTPUT);
  pinMode(FAN_FIRST_PIN + 1, OUTPUT);
  setSpeed(loadState(FAN_FIRST_PIN));
}

void setup()
{

}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("houseFanControl", "0.1");
	present(FAN_FIRST_PIN, S_DIMMER);
}


void loop()
{

}

void setSpeed(fanSpeed speed) {
  if (speed == AUTOMATIC) {
    speed = DEFAULT_SPEED;
  }
  
  if (speed == MIN) {
    digitalWrite(FAN_FIRST_PIN, RELAY_OFF);
    digitalWrite(FAN_FIRST_PIN + 1, RELAY_OFF);
  }
  else if (speed == MAX) {
    digitalWrite(FAN_FIRST_PIN, RELAY_OFF);
    digitalWrite(FAN_FIRST_PIN + 1, RELAY_ON);
  }
  else { //Speed == normal
    digitalWrite(FAN_FIRST_PIN, RELAY_ON);
    digitalWrite(FAN_FIRST_PIN + 1, RELAY_OFF);
  }
  saveState(FAN_FIRST_PIN, speed);
}

void receive(const MyMessage &message)
{
	// We only expect one type of message from controller. But we better check anyway.
	if (message.type==V_PERCENTAGE) {
    setSpeed(percentageToFanSpeed(message.getInt()));
	}
  Serial.println(message.type);
}

fanSpeed percentageToFanSpeed(int percentage) {
  if (percentage == 0)
    return 1;
  else if (percentage >= 66)
    return 2;
  else if (percentage >= 33)
    return 3;
  else
    return 1;
}

