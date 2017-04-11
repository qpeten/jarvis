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

// Enable debug prints to serial monitor
#define MY_DEBUG


// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level.
#define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable serial gateway
#define MY_GATEWAY_SERIAL

// Define a lower baud rate for Arduino's running on 8 MHz (Arduino Pro Mini 3.3V & SenseBender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE

// Inverses behavior of inclusion button (if using external pullup)
//#define MY_INCLUSION_BUTTON_EXTERNAL_PULLUP

// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Inverses the behavior of leds
//#define MY_WITH_LEDS_BLINKING_INVERSE

// Flash leds on rx/tx/err
// Uncomment to override default HW configurations
//#define MY_DEFAULT_ERR_LED_PIN 4  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  6  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  5  // the PCB, on board LED

#include <MySensors.h>

#define CHILD_ID_LIGHT 1 //Pin used to control the light's relay
#define PIN_LIGHT_GARAGE 4
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay
#define INPUT_PIN_SWITCH 2 // Pin used to detect the switch state
#define SWITCH_CHANGE_DEBOUNCE_MILLIS 80
#define SWITCH_MAX_TIME_BETWEEN_KNOCKS 750
#define LIGHT_ON_LONG_TIME 60*60*1000
#define LIGHT_ON_SHORT_TIME 60*1000

typedef enum lightStatus {
  OFF=0,
  ON_SHORT=1,
  ON_LONG=2
} lightStatus;

bool lastSwitchState;
short nbrKnocks=1;
unsigned long lastSwitchChange=0;
unsigned long lastLightOn=0;
lightStatus garageLightStatus = OFF;

MyMessage lightGarage(CHILD_ID_LIGHT, S_BINARY);

void before() {
  pinMode(PIN_LIGHT_GARAGE, OUTPUT);
  digitalWrite(PIN_LIGHT_GARAGE, loadState(PIN_LIGHT_GARAGE)?RELAY_ON:RELAY_OFF);
  
  pinMode(INPUT_PIN_SWITCH, INPUT);
  lastSwitchState = digitalRead(INPUT_PIN_SWITCH);
}

void setup()
{
	
}

void presentation()
{
	sendSketchInfo("Relay", "1.0");

  present(PIN_LIGHT_GARAGE, S_BINARY);
}

void loop()
{
	manageSwitch();
  manageLight();
}

void manageLight(){
  
}

bool hasSwitchChanged() {
  if (millis() - lastSwitchState > SWITCH_CHANGE_DEBOUNCE_MILLIS &&
      lastSwitchChange != digitalRead(INPUT_PIN_SWITCH)) {
    lastSwitchState = !lastSwitchState;
    return true;
  }
  return false;
}

void manageSwitch() {
  bool switchTriggered = hasSwitchChanged();
  
  if (switchTriggered) {
    turnLightOn(true);
    manageKnocks();
    if (nbrKnocks == 3) {
      turnLightOn(false);
    }
  }  
}

void manageKnocks() {
  if (nbrKnocks == 0) {
    nbrKnocks = 1;
    lastSwitchChange = millis();
  }
  else if (abs(millis() - lastSwitchChange) < SWITCH_MAX_TIME_BETWEEN_KNOCKS*nbrKnocks) {
    nbrKnocks++;
  }
  else {
    nbrKnocks = 0;
  }
}

void changeLightState(bool newState) {
  digitalWrite(PIN_LIGHT_GARAGE, newState);
  saveState(PIN_LIGHT_GARAGE, newState);
  send(lightGarage.set(newState));  
}

//onShort should be set to false if the light is supposed to stay on for a long time
void turnLightOn(bool onShort) {
  changeLightState(true);
  switch (onShort) {
    case true:  garageLightStatus = ON_SHORT;  break;
    case false: garageLightStatus = ON_LONG;   break;
  }
  lastLightOn = millis();
}

void turnLightOff() {
  changeLightState(false);
  garageLightStatus = OFF;
}

void receive(const MyMessage &message)
{
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_STATUS) {
    if (message.sensor == PIN_LIGHT_GARAGE) {
      // Change relay state
      digitalWrite(PIN_LIGHT_GARAGE, message.getBool()?RELAY_ON:RELAY_OFF);
      // Store state in eeprom
      saveState(message.sensor, message.getBool());
      // Write some debug info
      Serial.print("Incoming change for sensor:");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println(message.getBool());
    }
    else {
      Serial.print("Error: Received wrong sensor number.");
    }
  }
}
