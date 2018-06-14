/**
 * This node is intended to be two motion sensors
 * 
 */

#define PIN_MOTION_BED_INPUT D1
#define PIN_MOTION_DOOR_INPUT D2
#define DEBOUNCE_TIME_MOTION_MS 4500


#include <ESP8266WiFi.h>
#include <PubSubClient.h>



IPAddress MQTTserver(192, 168, 1, 150);

WiFiClient wifiClient;
PubSubClient client(wifiClient);

long lastReconnectAttempt = 0;

unsigned long lastMotionDetectedBed = 0;
unsigned long lastMotionDetectedDoor = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("Restarting");
  client.setServer(MQTTserver, 1883);
  client.setCallback(callback);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to Wifi");
  
  //WiFi.mode(WIFI_STA);
  //WiFi.begin(ssid, password);
  WiFi.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.println(WiFi.status());
  }
  
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  
  lastReconnectAttempt = 0;
  pinMode(PIN_MOTION_BED_INPUT, INPUT_PULLUP);
  pinMode(PIN_MOTION_DOOR_INPUT, INPUT_PULLUP);
  Serial.println("Setup finished.");
}

void loop()
{
  manageMQTTConnexion();
  manageMotionSensor();
}

    

 void manageMotionSensor() {
  if (millis() > lastMotionDetectedBed + DEBOUNCE_TIME_MOTION_MS && digitalRead(PIN_MOTION_BED_INPUT)) {
    client.publish("/jarvis/out/status/etage/chQuentin/motionBed", "MVMT");
    lastMotionDetectedBed = millis();
  }
  if (millis() > lastMotionDetectedDoor + DEBOUNCE_TIME_MOTION_MS && digitalRead(PIN_MOTION_DOOR_INPUT)) {
    client.publish("/jarvis/out/status/etage/chQuentin/motionDoor", "MVMT");
    lastMotionDetectedDoor = millis();
  }
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
  if (client.connect("arduino-ChQuentin-motion")) {
    //client.subscribe("/jarvis/in/command/etage/chQuentin/#");
  }
  return client.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {

}


