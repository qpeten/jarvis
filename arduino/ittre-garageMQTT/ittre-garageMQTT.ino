/**
 * This node is supposed to be connected via an W5100 ethernet board.
 * 
 * Outputs :
 *   - Light in the garage "overheadlight"
 *   - Boiler parents "boilerparents"
 *   - Boiler Aymeric "boileraymeric"
 * Inputs :
 *   - Current sensor on garage door "garagedoormovement"
 *   - Switch to control lights "lightswitch"
 */

#include <SPI.h>
#include "Ethernet.h"
#include <PubSubClient.h>


/**
 * A4 = GND
 * A5 = 5V
 */
#define PIN_LIGHT_RELAY 4
#define PIN_LIGHT_TOGGLE_MILLIS 25  //Time the relay has to be kept up for the light to actually change
#define PIN_BOILER_PARENTS_RELAY 5
#define PIN_BOILER_AYMERIC_RELAY 6
#define PIN_LIGHT_OUT_RELAY 7
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay
#define PIN_LIGHT_INPUT 2 // Pin used to detect the switch state
#define PIN_CURRENT_SENSOR 14
#define CURRENT_SENSOR_THRES 8
#define CURRENT_SENSOR_SMOOTHING_NBR_READINGS 1000
#define LIGHT_CHANGE_DEBOUNCE_MILLIS 50
#define CURRENT_SENSOR_DEBOUNCE_MILLIS 14000 //Should be slightly longer than the time it takes to open or close the door

byte mac[] = {0xDE, 0xAA, 0xAA, 0x01, 0x00, 0x00};
IPAddress ip = (192, 168, 1, 50);
IPAddress MQTTserver(192, 168, 1, 150);

bool lastLightState = digitalRead(PIN_LIGHT_RELAY);
unsigned long lastLightChange = 0;
unsigned long lastCurrentSensorDetected = 0;
unsigned long lastLightToggleStart = 0;
EthernetClient ethClient;
PubSubClient client(ethClient);
unsigned long MQTTLastReconnectAttempt = 0;
unsigned long actualMesure = 0;
unsigned int nbrMesures = 0;

void setup() {
  pinMode(PIN_LIGHT_RELAY, OUTPUT);
  pinMode(PIN_LIGHT_OUT_RELAY, OUTPUT);
  pinMode(PIN_BOILER_PARENTS_RELAY, OUTPUT);
  pinMode(PIN_BOILER_AYMERIC_RELAY, OUTPUT);
  pinMode(PIN_LIGHT_INPUT, INPUT_PULLUP);
  pinMode(PIN_CURRENT_SENSOR, INPUT);


  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  digitalWrite(A4, LOW);
  digitalWrite(A5, HIGH);
  
  setRelay(PIN_LIGHT_RELAY, false);
  setRelay(PIN_LIGHT_OUT_RELAY, true);
  setRelay(PIN_BOILER_PARENTS_RELAY, true);
  setRelay(PIN_BOILER_AYMERIC_RELAY, true);
  lastLightState = digitalRead(PIN_LIGHT_INPUT);

  Serial.begin(115200);
  Serial.println("Has just started.");

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP. Falling back to static IP.");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }

  //Setting MQTT up
  client.setServer(MQTTserver, 1883);
  client.setCallback(MQTTMessageReceived);
  
  // give the Ethernet shield a second to initialize:
  delay(1000);
}

void loop() {
  manageMQTTConnexion();
  manageActualLightChange();
  manageCurrentSensor();
  manageLightOutputToggle();
}

void manageLightOutputToggle() {
  if (millis() - lastLightToggleStart > PIN_LIGHT_TOGGLE_MILLIS && digitalRead(PIN_LIGHT_RELAY)) {
    Serial.println(" Turning relay off.");
    digitalWrite(PIN_LIGHT_RELAY, false);
  }
}

void setRelay(byte pin, bool state) {
  if (pin == PIN_LIGHT_RELAY) {
    toggleLight(state);
  }
  else if (pin == PIN_BOILER_PARENTS_RELAY) {
    digitalWrite(PIN_BOILER_PARENTS_RELAY, state ? RELAY_ON : RELAY_OFF);
    client.publish("/jarvis/out/state/rez/garage/BoilerParents", getTruthValueFromBool(state));
  }
  else if (pin == PIN_BOILER_AYMERIC_RELAY) {
    digitalWrite(PIN_BOILER_AYMERIC_RELAY, state ? RELAY_ON : RELAY_OFF);
    client.publish("/jarvis/out/state/rez/garage/BoilerAymeric", getTruthValueFromBool(state));
  }
  else if (pin == PIN_LIGHT_OUT_RELAY) {
    digitalWrite(PIN_LIGHT_OUT_RELAY, state ? !RELAY_ON : !RELAY_OFF);
    client.publish("/jarvis/out/state/rez/garage/ParkingLight", getTruthValueFromBool(state));
  }
  else {
    Serial.println("Wrong pin for setRelay");
  }
}

const char* getTruthValueFromBool (bool input) {
  return input ? "ON" : "OFF";
}

void manageCurrentSensor() {
  if (currentSensorTriggered() && millis() - lastCurrentSensorDetected > CURRENT_SENSOR_DEBOUNCE_MILLIS) {
    client.publish("/jarvis/out/state/rez/garage/GarageDoorMovement","MVMT");
  }
}

bool currentSensorTriggered() {
  if (nbrMesures > CURRENT_SENSOR_SMOOTHING_NBR_READINGS) {
    int val = actualMesure/nbrMesures;
    actualMesure = 0;
    nbrMesures = 0;
    return val > CURRENT_SENSOR_THRES;
  }
  else {
    actualMesure += abs((int)analogRead(PIN_CURRENT_SENSOR) - (int)511);
    nbrMesures++;
    return false;
  }
}

bool hasActualLightChanged() {
  bool currentSwitchState = digitalRead(PIN_LIGHT_INPUT);
  if (millis() - lastLightChange > LIGHT_CHANGE_DEBOUNCE_MILLIS && lastLightState != currentSwitchState) {
    lastLightState = currentSwitchState;
    lastLightChange = millis();
    digitalWrite(PIN_LIGHT_RELAY, false); //We can stop the toggle operation
    return true;
  }
  else {
    return false;
  }
}

void manageActualLightChange() {
  if (hasActualLightChanged()) {
    client.publish("/jarvis/out/state/rez/garage/OverheadLight", lastLightState ? "ON" : "OFF");
  }
}

void toggleLight(bool newState) {
  Serial.print("Toggle light. Pin = ");
  Serial.print(digitalRead(PIN_LIGHT_RELAY));
  Serial.print(" New state = ");
  Serial.print(newState);
  if ((digitalRead(PIN_LIGHT_RELAY) == (RELAY_ON == 1)) xor newState) {
    Serial.println(" Turning relay on.");
    digitalWrite(PIN_LIGHT_RELAY, true);
    lastLightToggleStart = millis();
  }
}

void MQTTMessageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.println("Received message");
  if(strcmp(topic, "/jarvis/in/command/rez/garage/OverheadLight") == 0)
    setRelay(PIN_LIGHT_RELAY, getTruthValue(payload));
  else if(strcmp(topic, "/jarvis/in/command/rez/garage/ParkingLight") == 0)
    setRelay(PIN_LIGHT_OUT_RELAY, getTruthValue(payload));
  else if(strcmp(topic, "/jarvis/in/command/rez/garage/BoilerParents") == 0)
    setRelay(PIN_BOILER_PARENTS_RELAY, getTruthValue(payload));
  else if(strcmp(topic, "/jarvis/in/command/rez/garage/BoilerAymeric") == 0)
    setRelay(PIN_BOILER_AYMERIC_RELAY, getTruthValue(payload));
}

//If unclear, defaults to false
bool getTruthValue(byte* string) {
  if (string[0] == '1') {
	  return true;
  }
  else {
	  return false;
  }
}

void manageMQTTConnexion() {
  if (!client.connected()) {
    long now = millis();
    if (now - MQTTLastReconnectAttempt > 5000) {
      MQTTLastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        MQTTLastReconnectAttempt = 0;
      }
    }
  } else {
    // We are still connected
    client.loop();
  }
}

boolean reconnect() {
  if (client.connect("rez-Garage")) {
    // Once connected, publish an announcement
    client.publish("/jarvis/out/state/rez/garage","New connection");
    // ... and (re)subscribe
    client.subscribe("/jarvis/in/command/rez/garage/#");
  }
  return client.connected();
}

