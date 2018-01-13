/**
 * This node is intended to control the light above the sink in Quentin's room, the audio amplifiers and be a motion sensor
 * 
 */

#define PIN_SWITCH_INPUT 8
#define PIN_LED_OUTPUT 3
#define PIN_PWR_OUTPUT 2
#define NUM_READINGS_MQTT_LED_DIMMER 50 //How many sample to smooth the input value of the LEDs
#define TIME_READINGS_MQTT_LED_DIMMER_MS 8 //How long to wait befor readings (if no new reading is received, the previous value is used)
#define DEBOUNCE_TIME_SWITCH_MS 100
#define DEBOUNCE_TIME_MOTION_MS 4500


#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>



byte mac[]    = {  0xDE, 0xAD, 0xAA, 0xAA, 0x72, 0x19 };
IPAddress ip(192, 168, 1, 210);
IPAddress server(192, 168, 1, 150);

EthernetClient ethClient;
PubSubClient client(ethClient);

long lastReconnectAttempt = 0;

unsigned char lastInputs[NUM_READINGS_MQTT_LED_DIMMER];
unsigned char lastInputsIndex = 0;
int lastInputsTotal = 0;
unsigned long lastInputsLastReading = 0;
unsigned char targetValuePercentage = 0;

bool lastValueSwitch;
unsigned long lastSwitchToggle = 0;

unsigned long lastMotionDetected = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("Restarting");
  client.setServer(server, 1883);
  client.setCallback(callback);
  for (unsigned char i = 0; i < NUM_READINGS_MQTT_LED_DIMMER; i++) {
    lastInputs[i] = 0;
  }

  delay(1000);
  Ethernet.begin(mac);
  
  Serial.println("LAN done.");
  delay(1000);
  lastReconnectAttempt = 0;
  pinMode(PIN_LED_OUTPUT, OUTPUT);
  pinMode(PIN_SWITCH_INPUT, INPUT_PULLUP);
//  pinMode(PIN_MOTION_INPUT, INPUT_PULLUP);
  pinMode(PIN_PWR_OUTPUT, OUTPUT);
  digitalWrite(PIN_LED_OUTPUT, false);
  lastValueSwitch = digitalRead(PIN_SWITCH_INPUT);
  digitalWrite(PIN_PWR_OUTPUT, true); //Power supply STDBY
  Serial.println("Setup finished.");
}

void loop()
{
  manageMQTTConnexion();
  manageSwitch();
  manageLEDDimmer();
  //manageMotionSensor();
  Ethernet.maintain();
}

    

/* void manageMotionSensor() {
  if (millis() > lastMotionDetected + DEBOUNCE_TIME_MOTION_MS && digitalRead(PIN_MOTION_INPUT)) {
    client.publish("/jarvis/out/status/etage/chQuentin/motion", "MVMT");
    lastMotionDetected = millis();
  }
} */

void manageSwitch() {
  if (millis() > lastSwitchToggle + DEBOUNCE_TIME_SWITCH_MS) {
    bool newValue = digitalRead(PIN_SWITCH_INPUT);
    if (lastValueSwitch != newValue) {
      toggleLED();
      lastValueSwitch = newValue;
      lastSwitchToggle = millis();
    }
  }
}

void toggleLED() {
  unsigned char newValue = 100;
  if (lastInputsTotal/NUM_READINGS_MQTT_LED_DIMMER > 0)
    newValue = 0;

  lastInputsAddNewReading(newValue);
  
  //client.publish("/jarvis/out/status/etage/chQuentin/sideLightBrightness", newValue);
}

void manageLEDDimmer() {
  if (millis() > lastInputsLastReading + TIME_READINGS_MQTT_LED_DIMMER_MS) {
    lastInputsRefresh();
    
    if (lastInputsTotal > 0)
      digitalWrite(PIN_PWR_OUTPUT, false);
    else
      digitalWrite(PIN_PWR_OUTPUT, true);
      
    analogWrite(PIN_LED_OUTPUT, lastInputsTotal/NUM_READINGS_MQTT_LED_DIMMER); 
    lastInputsLastReading = millis();
  }
}

void lastInputsRefresh() {
  unsigned char readIndex = 0;
  if (lastInputsIndex == 0)
    readIndex = NUM_READINGS_MQTT_LED_DIMMER - 1;
  else
    readIndex = lastInputsIndex - 1;

  lastInputsAddNewReadingToTable(lastInputs[readIndex]);
}

void lastInputsAddNewReading(unsigned char value) {
  if (value > 100)
    value = 100;
  else if (value < 0)
    value = 0;

  targetValuePercentage = value;
  char buffer[4] = "";
  sprintf(buffer, "%d", value);
  client.publish("/jarvis/out/status/etage/chQuentin/sideLightBrightness", buffer);
  
  unsigned char output = 0;
  
  if (value > 10)
    output = map(value, 11, 100, 31, 255);
  else if (value == 1)
    output = 1;
  else if (value == 0)
    output = 0;
  else
    output = map(value, 2, 10, 2, 30);

  lastInputsAddNewReadingToTable(output);
}

void lastInputsAddNewReadingToTable(unsigned char value) {
  lastInputsTotal -= lastInputs[lastInputsIndex];
  lastInputs[lastInputsIndex] = value;
  lastInputsTotal += lastInputs[lastInputsIndex];
  lastInputsIndex++;
  
  if (lastInputsIndex >= NUM_READINGS_MQTT_LED_DIMMER)
    lastInputsIndex = 0;
}

void manageMQTTConnexion() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }
}

boolean reconnect() {
  if (client.connect("arduino-ChQuentin")) {
    client.subscribe("/jarvis/in/command/etage/chQuentin/#");
  }
  return client.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
  char* newString = (char*)malloc(length);
  memcpy(newString, payload, length);
  newString[length] = '\0';
  if (String(topic).indexOf("sideLightBrightness") > 0)
    parseSideLightMQTTMessage(newString);
  free(newString);
}

void parseSideLightMQTTMessage(char* payload) {
  lastInputsAddNewReading(atoi(payload));
}



