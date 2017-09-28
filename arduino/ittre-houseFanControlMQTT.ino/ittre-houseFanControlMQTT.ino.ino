/*
 * Code based on Superhouse.tv's https://github.com/SuperHouse/BasicOTARelay/blob/master/BasicOTARelay.ino
 * designed to be run on a Sonoff with 2 relays
 * 
 */
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

/* WiFi Settings */
const char* ssid     = "xxxxxxxxxx";
const char* password = "xxxxxxxxxx";

#define FAN_FIRST_PIN 12
#define RELAY_ON 0  // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 // GPIO value to write to turn off attached relay


/* MQTT Settings */
const char* mqttTopic = "/jarvis/in/command/grenier/parents/ventilation";   // MQTT topic
IPAddress broker(192,168,1,150);          // Address of the MQTT broker
#define CLIENT_ID "grenier-ventilation"         // Client ID to send to the broker


WiFiClient wificlient;
PubSubClient client(wificlient);

/**
 * MQTT callback to process messages
 */
void callback(char* topic, byte* payload, unsigned int length) {
  if (payload[0] == '1' || payload[0] == '0') { //Min speed
    digitalWrite(FAN_FIRST_PIN, RELAY_OFF);
    digitalWrite(FAN_FIRST_PIN + 1, RELAY_OFF);
    client.publish("/jarvis/out/state/grenier/parents/ventilation","1");
  }
  else if (payload[0] == '3') { //Max speed
    digitalWrite(FAN_FIRST_PIN, RELAY_OFF);
    digitalWrite(FAN_FIRST_PIN + 1, RELAY_ON);
    client.publish("/jarvis/out/state/grenier/parents/ventilation","2");
  }
  else { //Speed == normal
    digitalWrite(FAN_FIRST_PIN, RELAY_ON);
    digitalWrite(FAN_FIRST_PIN + 1, RELAY_OFF);
    client.publish("/jarvis/out/state/grenier/parents/ventilation","3");
  }
}

/**
 * Attempt connection to MQTT broker and subscribe to command topic
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("WiFi begun");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Proceeding");
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR   ) Serial.println("OTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR  ) Serial.println("OTA Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("OTA Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA Receive Failed");
    else if (error == OTA_END_ERROR    ) Serial.println("OTA End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /* Prepare MQTT client */
  client.setServer(broker, 1883);
  client.setCallback(callback);
}

void loop() {
  ArduinoOTA.handle();
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, password);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
  }
  
  if (client.connected())
  {
    client.loop();
  }
}
