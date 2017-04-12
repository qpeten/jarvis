/**
 * Used to control a fan from a PWM signal.
 * 
 * The fan is controlled by 4 relays. Maximum one relay may be active at anytime.
 * The script allows up to MAX_NUMBER_OF_SWITCH_PER_MINUTE changes per minute. The timer is reset each time the PWM signal indicates the fan should be off.
 */

#define RELAY_1 4  //4 // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 4 //4 // Total number of attached relays
#define RELAY_ON 0  //0 // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 //1 // GPIO value to write to turn off attached relay
#define PWM_INPUT 23 //23
#define MAX_NUMBER_OF_SWITCH_PER_MINUTE 9 //9
#define TIMEOUT_MILLIS 60000 //60000

int actualLevel = -1;
unsigned long lastSwitch[MAX_NUMBER_OF_SWITCH_PER_MINUTE];
short lastSwitchIndex = 0;

void setup() {
  pinMode(PWM_INPUT, INPUT);
  for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
    pinMode(RELAY_1 + i, OUTPUT);
  }
  initializeLastSwitch();
  turnOffAllRelay();
  Serial.println("Ready");
}

void loop() {
  //Serial.println(getLevel(takeMesure())); //@@@DEBUG
  int newLevel = getLevel(takeMesure());
  if (newLevel == 0) {
    switchRelay(0);
  }
  if (newLevel != actualLevel && OKToChange()) {
    switchRelay(newLevel);
  }
}

void initializeLastSwitch() {
  for (int i = 0; i< MAX_NUMBER_OF_SWITCH_PER_MINUTE; i++){
    lastSwitch[i] = -TIMEOUT_MILLIS;
  }
}

void newSwitch() {
  lastSwitch[nextLastSwitchIndex()] = millis();
}

int nextLastSwitchIndex() {
  lastSwitchIndex++;
  if (lastSwitchIndex >= MAX_NUMBER_OF_SWITCH_PER_MINUTE){
    lastSwitchIndex = 0;
  }
  return lastSwitchIndex;
}

bool OKToChange() {
  return getNumberOfSwitchesLastMinute() < MAX_NUMBER_OF_SWITCH_PER_MINUTE;
}

short getNumberOfSwitchesLastMinute() {
  unsigned long now = millis();
  short numberOfSwitches = 0;
  for (int i = 0; i< MAX_NUMBER_OF_SWITCH_PER_MINUTE; i++){
    if (now - lastSwitch[i] < TIMEOUT_MILLIS){
      numberOfSwitches++;
    }
  }
  return numberOfSwitches;
}

int takeMesure() {
  long mesure=0;
  // read the input on analog pin 0:
  for (int i = 0; i< 500 ; i++){
    mesure += analogRead(A0);
  }
  return (int) (mesure /= 500);
}

int getLevel(long mesure) {
  /*assert(mesure >= 0);
  assert(mesure <= 1024);*/
  if (mesure < 700) {
    return 0;
  }
  else if (mesure < 825) {
    return 1;
  }
  else if (mesure < 900) {
    return 2;
  }
  else if (mesure < 1000) {
    return 3;
  }
  else { //1023+
    return 4;
  }
}

void switchRelay(int level) {
  /*assert(level >= 0);
  assert(level <= NUMBER_OF_RELAYS);*/
  turnOffAllRelay();
  if (level != 0) {
    digitalWrite(RELAY_1 + level - 1, RELAY_ON);
  }
  else {
    initializeLastSwitch();
  }
  newSwitch();
  actualLevel = level;
}

void turnOffAllRelay() {
  for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
    digitalWrite(RELAY_1 + i, RELAY_OFF);
  }
  delay(100); //To be sure all relays are off before doing anything else (ex: switching an other relay on.)
  actualLevel = 0;
}
