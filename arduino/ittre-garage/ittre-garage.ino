/**
 * This node is meant to output the value mesured on on pin to Serial
 * 
 * Outputs :
 *   - Serial
 * Inputs :
 *   - Current sensor on garage door "garagedoormovement"
 */

#define PIN_CURRENT_SENSOR 14
/*#define CURRENT_SENSOR_THRES 8
#define CURRENT_SENSOR_SMOOTHING_NBR_READINGS 5000
#define CURRENT_SENSOR_DEBOUNCE_MILLIS 21000 //Should be slightly longer than the time it takes to open or close the door. Default : 21000

unsigned long lastCurrentSensorDetected = 0;
unsigned long actualMesure = 0;
unsigned int nbrMesures = 0;
*/
void setup() {
  pinMode(PIN_CURRENT_SENSOR, INPUT);

  Serial.begin(115200);
}

void loop() {
  //manageCurrentSensor();
  Serial.println(analogRead(PIN_CURRENT_SENSOR));
}
/*
void manageCurrentSensor() {
  if (currentSensorTriggered() && millis() - lastCurrentSensorDetected > CURRENT_SENSOR_DEBOUNCE_MILLIS) {
    lastCurrentSensorDetected = millis();
    Serial.println("MVMT");
  }
}

bool currentSensorTriggered() {
  if (nbrMesures > CURRENT_SENSOR_SMOOTHING_NBR_READINGS) {
    //Serial.println("Cycle complete");
    int val = actualMesure/nbrMesures;
    actualMesure = 0;
    nbrMesures = 0;
    return val > CURRENT_SENSOR_THRES;
  }
  else {
    actualMesure += abs((int)analogRead(PIN_CURRENT_SENSOR) - (int)511);
    //Serial.println(analogRead(PIN_CURRENT_SENSOR));
    nbrMesures++;
    return false;
  }
}
*/
