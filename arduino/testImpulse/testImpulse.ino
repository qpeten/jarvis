char receivedChar;
bool newData;

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
 digitalWrite(4, HIGH);
 delay(50);
 digitalWrite(4, LOW);
 newData = false;
 }
}
