char receivedChar;
bool newData;
int delayTest = 50;

void setup() {
  // put your setup code here, to run once:
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  Serial.begin(115200);
  Serial.println("<Arduino is ready>");
}

void loop() {
 recvOneChar();
 showNewData();
}

void recvOneChar() {
 if (Serial.available() > 0) {
 receivedChar = Serial.read();
 newData = true;
 }
}

void showNewData() {
 if (newData == true) {
  if (receivedChar == '1') {
    delayTest = 10;
  }
  else if (receivedChar == '2') {
    delayTest = 20;
  }
  else if (receivedChar == '3') {
    delayTest = 50;
  }
  else if (receivedChar == '4') {
    delayTest = 100;
  }
  else if (receivedChar == '0') {
    delayTest = 5;
  }
 digitalWrite(4, HIGH);
 delay(delayTest);
 digitalWrite(4, LOW);
 newData = false;
 }
}
