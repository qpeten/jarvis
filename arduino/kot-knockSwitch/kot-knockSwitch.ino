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
 * Version 0.5 - Quentin Peten
 *
 * DESCRIPTION
 * Sketch used to register knock sequences
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <MySensors.h>

#define CHILD_ID_2KNOCKS 2
#define CHILD_ID_3KNOCKS 3
#define CHILD_ID_4KNOCKS 4
#define CHILD_ID_5KNOCKS 5

const int knockSensor = 5; // (Digital 5) for using the microphone digital output (tune knob to register knock)

/*Tuning constants. Changing the values below changes the behavior of the device.*/
int threshold = 3; // Minimum signal from the piezo to register as a knock. Higher = less sensitive. Typical values 1 - 10
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock. Typical values 10-30
const int averageRejectValue = 15; // If the average timing of all the knocks is off by this percent we don't unlock. Typical values 5-20
const int knockFadeTime = 175;     // Milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)
const int normalTimeDiff = 500; //Normal time between knocks, in ms.
const int maximumKnocks = 10;      // Maximum number of knocks to listen for.
const int knockComplete = 1000; // Longest time to wait for a knock before we assume that it's finished. (milliseconds)

int knockReadings[maximumKnocks];    // When someone knocks this array fills with the delays between knocks.
int knockSensorValue = 0;            // Last reading of the knock sensor.
int startTime = 0;  //Used to record the first knock

MyMessage msg2Knocks(CHILD_ID_2KNOCKS, V_SCENE_ON);
MyMessage msg3Knocks(CHILD_ID_3KNOCKS, V_SCENE_ON);
MyMessage msg4Knocks(CHILD_ID_4KNOCKS, V_SCENE_ON);
MyMessage msg5Knocks(CHILD_ID_5KNOCKS, V_SCENE_ON);

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("QKnock Switch", "0.6");

	// Register all sensors to gateway (they will be created as child devices)
	present(CHILD_ID_2KNOCKS, S_SCENE_CONTROLLER);
  present(CHILD_ID_3KNOCKS, S_SCENE_CONTROLLER);
  present(CHILD_ID_4KNOCKS, S_SCENE_CONTROLLER);
  present(CHILD_ID_5KNOCKS, S_SCENE_CONTROLLER);
}

void setup() {
  pinMode(knockSensor, INPUT);
  
}
void loop()
{
	knockSensorValue = digitalRead(knockSensor);
  if (knockSensorValue == 1) {
    startTime = millis();
    knockDelay();
    listenToKnock();
  }
}

// Records the timing of knocks.
void listenToKnock()
{
  int i = 0;
  // First reset the listening array.
  for (i=0; i < maximumKnocks; i++) {
    knockReadings[i] = 0;
  }

  int currentKnockNumber = 0;               // Position counter for the array.
  int now = millis();

  do {                                      // Listen for the next knock or wait for it to timeout.
    knockSensorValue = digitalRead(knockSensor);

    if (knockSensorValue == 1) {                  // Here's another knock. Save the time between knocks.
      Serial.println("knock");

      now=millis();
      knockReadings[currentKnockNumber] = now - startTime;
      currentKnockNumber ++;
      startTime = now;

      knockDelay();
    }

    now = millis();

    // Stop listening if there are too many knocks or there is too much time between knocks.
  } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));

    sendMessage(numberOfKnocks());
}

void sendMessage(int numberOfKnocks) {
  switch (numberOfKnocks) {
    case 2: send(msg2Knocks.set(true)); break;
    case 3: send(msg3Knocks.set(true)); break;
    case 4: send(msg4Knocks.set(true)); break;
    case 5: send(msg5Knocks.set(true)); break;
    default:Serial.println("Invalid knock sequence"); break;
  }
}

// Checks to see if the knock sequence is somewhat regular.
// Returns the number of knocks or -1 if the sequence is not regular
int numberOfKnocks()
{
  int i = 0;

  int currentKnockCount = 0;
  int maxKnockInterval = 0;               // We use this later to normalize the times.

  for (i=0; i<maximumKnocks; i++) {
    if (knockReadings[i] > 0) {
      currentKnockCount++;
    }

    if (knockReadings[i] > maxKnockInterval) {  // Collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }

  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (ie: if you do the same pattern slow or fast it should still be valid.)
      This makes it less picky, which while making it less secure can also make it
      less of a pain to use if your tempo is a little slow or fast.
  */
  int totaltimeDifferences = 0;
  int timeDiff = 0;
  for (i=0; i < maximumKnocks; i++) {   // Normalize the times
    knockReadings[i]= map(knockReadings[i], 0, maxKnockInterval, 0, 100);
    if (knockReadings[i] == 0) {  //We are at the end of the sequence
      break;
    }
    timeDiff = abs(knockReadings[i] - 100);
    if (timeDiff > rejectValue) {       // Individual value too far out of whack. No access for this knock!
      return -1;
    }
    totaltimeDifferences += timeDiff;
  }
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences / currentKnockCount > averageRejectValue) {
    return -2;
  }

  return currentKnockCount + 1;
}

void knockDelay()
{
  int itterations = (knockFadeTime / 20);      // Wait for the peak to dissipate before listening to next one.
  for (int i=0; i < itterations; i++) {
    delay(10);
    analogRead(
        knockSensor);                  // This is done in an attempt to defuse the analog sensor's capacitor that will give false readings on high impedance sensors.
    delay(10);
  }
}
