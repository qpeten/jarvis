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
#define MY_DEBUG

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

#define MY_NODE_ID 3

#include <MySensors.h>

#define PIN_FILTER_RELAY 4
#define PIN_FOUNTAIN_RELAY 5
#define PIN_LIGHT_RELAY 6
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

void before() {
  pinMode(PIN_FILTER_RELAY, OUTPUT);
  pinMode(PIN_FOUNTAIN_RELAY, OUTPUT);
  pinMode(PIN_LIGHT_RELAY, OUTPUT);
  digitalWrite(PIN_FILTER_RELAY, RELAY_ON);
  digitalWrite(PIN_FOUNTAIN_RELAY, RELAY_OFF);
	digitalWrite(PIN_LIGHT_RELAY, RELAY_OFF);
}

void setup()
{
  
}

void presentation()
{
	sendSketchInfo("Etang", "0.1");
  present(PIN_FILTER_RELAY, S_BINARY, "Pompe et UV étang");
  present(PIN_FOUNTAIN_RELAY, S_BINARY, "Fontaine étang");
  present(PIN_LIGHT_RELAY, S_BINARY, "Lumière étang");
}

void loop()
{
	
}

void setRelay(uint8_t pin, bool value) {
  digitalWrite(pin, value ? RELAY_ON : RELAY_OFF);
  saveState(pin, value);
}

void receive(const MyMessage &message)
{
  if (message.destination == MY_NODE_ID) {
    if (message.sensor == PIN_FILTER_RELAY) {
			setRelay(PIN_FILTER_RELAY, message.getBool());
    }
    else if (message.sensor == PIN_FOUNTAIN_RELAY) {
      setRelay(PIN_FOUNTAIN_RELAY, message.getBool());
    }
    else if (message.sensor == PIN_LIGHT_RELAY) {
      setRelay(PIN_LIGHT_RELAY, message.getBool());
    }
    else {
      Serial.print("Error: Received wrong sensor number.");
    }
  }
}
