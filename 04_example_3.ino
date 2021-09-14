#define PIN_LED 13 // LED 연결

void setup() {
  pinMode(PIN_LED, OUTPUT); // 출력 준비
}

void loop() {
  //LED ON
  digitalWrite(PIN_LED, 1);
  // 1초 기다리기
  delay(1000); 
  // LED OFF
  digitalWrite(PIN_LED, 0);
  // 1초 기다리기
  delay(1000);
}
