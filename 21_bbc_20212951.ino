
#include <Servo.h>
#define PIN_SERVO 10
#define PIN_IR A0
#define _DIST_ALPHA 0.3 // ema 필터 상수

Servo myservo;

int a, b; // unit: mm
float dist_ema, alpha;

// cali 구하기 위한 변수
float sum, sensorVal, volt, val;


void setup() {
  
// initialize serial port
  Serial.begin(57600);

  a = 100;
  b = 273;
  alpha = _DIST_ALPHA;
  dist_ema = 0;
}

float ir_distance(void){ // return value unit: mm
  sum = 0;
  for(int i = 0; i<100; i++){
    sum = sum + float(analogRead(PIN_IR));
  }
  sensorVal = sum / 100;  // 평균을 낸다
  volt = sensorVal * 5.0 / 1024.0;  // V로 환산
  val = pow(volt * (1 / 16.7647563), 1/(-0.85803107)) * 10.0;
  return val;
}

void loop() {
//  myservo.writeMicroseconds(1700);
  myservo.attach(PIN_SERVO);
  float raw_dist = ir_distance();
  float dist_cali = 100 + 300.0 / (b - a) * (raw_dist - a);
  
  if (dist_ema == 0){
    dist_ema = dist_cali;
  }
  else{
    dist_ema = alpha * dist_cali + (1-alpha) * dist_ema;
  }
  
  Serial.print("min:0 max:500 dist_cali: ");
  Serial.print(dist_cali);
  Serial.print(" dist_ema:");
  Serial.println(dist_ema);
  
  if (dist_ema > 255){
    myservo.writeMicroseconds(1550);
  }
  else{
    myservo.writeMicroseconds(1800);
  }
  delay(50);
}
