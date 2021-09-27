# define PINMODE 7 // LED 연결


void setup() {
  // put your setup code here, to run once:
  pinMode(7, OUTPUT);

}

void loop() {
  // LED ON
  digitalWrite(7, HIGH);
  // wait for 1 second
  delay(1000);
  // 껐다 켰다 5번 반복
  for(int i = 0; i < 5; i++){
    digitalWrite(7, 0);
    // wait for 0.1 second
    delay(100);
    // LED ON
    digitalWrite(7, 1);
    //wait for 0.1 second
    delay(100);
  }
  while(1){}
}
