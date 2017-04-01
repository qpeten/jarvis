#define RELAY_1  4  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 4 // Total number of attached relays
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay
#define PWM_INPUT 23

void setup() {
  Serial.begin(115200);
  pinMode(PWM_INPUT, INPUT);
  for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
    pinMode(RELAY_1 + i, OUTPUT);
  }

}

void loop() {
  long mesure=0;
  // read the input on analog pin 0:
  for (int i = 0; i< 500 ; i++){
    mesure += analogRead(A0);
  }
  mesure /= 500;
  Serial.println(getLevel(mesure)); //@@@DEBUG
}

int getLevel(long mesure) {
  /*assert(mesure >= 0);
  assert(mesure <= 1024);*/
  if (mesure < 700) {
    return 0;
  }
  else if (mesure < 875) {
    return 1;
  }
  else if (mesure < 920) {
    return 2;
  }
  else if (mesure < 990) {
    return 3;
  }
  else {
    return 4;
  }
}

void switchToRelay(int level) {
  /*assert(level >= 0);
  assert(level <= NUMBER_OF_RELAYS);*/
  turnOffAllRelay();
  if (level != 0) {
    digitalWrite(RELAY_1 + level, RELAY_ON);
  }
}

void turnOffAllRelay() {
  for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
    digitalWrite(RELAY_1 + i, RELAY_OFF);
  }
}
