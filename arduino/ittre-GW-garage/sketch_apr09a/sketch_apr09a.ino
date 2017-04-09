* REVISION HISTORY
 * Version 1.0 - Henrik Ekblad
 *
 * DESCRIPTION
 * Motion Sensor example using HC-SR501
 * http://www.mysensors.org/build/motion
 *
 */

// Enable debug prints
// #define MY_DEBUG


unsigned long SLEEP_TIME = 120000; // Sleep time between reports (in milliseconds)
#define DIGITAL_INPUT_SENSOR 3   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define CHILD_ID 1   // Id of the sensor child

// Initialize motion message


void setup()
{
    pinMode(DIGITAL_INPUT_SENSOR, INPUT);      // sets the motion sensor digital pin as input
}


void loop()
{
    // Read digital motion value
    bool tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH;

    Serial.println(tripped);
    send(msg.set(tripped?"1":"0"));  // Send tripped value to gw

    // Sleep until interrupt comes in on motion sensor. Send update every two minute.
    sleep(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), CHANGE, SLEEP_TIME);
}
