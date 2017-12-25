/**
 * This node is intended to control the light above the sink in Quentin's room, the audio amplifiers and be a motion sensor
 * 
 */

#define PIN_SWITCH_INPUT 2
#define PIN_LED_OUTPUT 3
#define PIN_MOTION_INPUT 8
#define PIN_PWR_OUTPUT 7
#define NUM_READINGS_MQTT_LED_DIMMER 50 //How many sample to smooth the input value of the LEDs
#define TIME_READINGS_MQTT_LED_DIMMER_MS 8 //How long to wait befor readings (if no new reading is received, the previous value is used)
#define DEBOUNCE_TIME_SWITCH_MS 100
#define DEBOUNCE_TIME_MOTION_MS 4500


#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>



byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 210);
IPAddress server(192, 168, 1, 150);

EthernetClient ethClient;
PubSubClient client(ethClient);

long lastReconnectAttempt = 0;

unsigned char lastInputs[NUM_READINGS_MQTT_LED_DIMMER];
unsigned char lastInputsIndex = 0;
int lastInputsTotal = 0;
unsigned long lastInputsLastReading = 0;

bool lastValueSwitch;
unsigned long lastSwitchToggle = 0;

unsigned long lastMotionDetected = 0;

void setup()
{
  client.setServer(server, 1883);
  client.setCallback(callback);

  for (unsigned char i = 0; i < NUM_READINGS_MQTT_LED_DIMMER; i++) {
    lastInputs[i] = 0;
  }

  Ethernet.begin(mac);
  delay(1000);
  lastReconnectAttempt = 0;
  Serial.begin(115200);
  Serial.println("Restarting");
  pinMode(PIN_LED_OUTPUT, OUTPUT);
  pinMode(PIN_SWITCH_INPUT, INPUT_PULLUP);
  pinMode(PIN_MOTION_INPUT, INPUT_PULLUP);
  pinMode(PIN_PWR_OUTPUT, OUTPUT);
  digitalWrite(PIN_LED_OUTPUT, false);
  lastValueSwitch = digitalRead(PIN_SWITCH_INPUT);
  digitalWrite(PIN_PWR_OUTPUT, true); //Power supply STDBY
}

void loop()
{
  manageMQTTConnexion();
  manageSwitch();
  manageLEDDimmer();
  manageMotionSensor();
}



void manageMotionSensor() {
  if (millis() > lastMotionDetected + DEBOUNCE_TIME_MOTION_MS && digitalRead(PIN_MOTION_INPUT)) {
    Serial.println("Tripped");
    lastMotionDetected = millis();
  }
}

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
  unsigned char newValue = 255;
  if (lastInputsTotal/NUM_READINGS_MQTT_LED_DIMMER > 0)
    newValue = 0;

  lastInputsAddNewReading(newValue);
  char buf [4];
  sprintf (buf, "%03i", newValue);
  client.publish("/jarvis/out/status/etage/chQuentin/sideLightBrightness", buf);
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

  lastInputsAddNewReading(lastInputs[readIndex]);
}
void lastInputsAddNewReading(unsigned char value) {
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
  unsigned char dimmerValue = atoi(payload);
  lastInputsAddNewReading(dimmerValue);
}



