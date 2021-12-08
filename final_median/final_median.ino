#include <Servo.h> // [2972] 서보 헤더파일 포함


/////////////////////////////
// Configurable parameters //
/////////////////////////////

// Arduino pin assignment
#define PIN_LED 9 // [1234] LED를 아두이노 GPIO 9번 핀에 연결
                  // [2345] ...하면 개선
#define PIN_SERVO 10 // [2951] servo moter를 아두이노 GPIO 10번 핀에 연결
#define PIN_IR A0      // [2961] 적외선센서를 아두이노 A0핀에 연결

#include "medianfilter.h" // 중위수 필터

// Framework setting
#define _DIST_TARGET 255 //[2967] 탁구공 위치까지의 거리를 255로 고정
         // [2952] 탁구공의 목표 위치를 25.5cm로 설정
#define _DIST_MIN 100 // [2972] 거리 센서가 인식 가능하게 설정한 최소 거리
#define _DIST_MAX 410 // [2972] 거리 센서가 인식 가능하게 설정한 최대 거리


// Distance sensor
#define _DIST_ALPHA 0.35   // [2959] ema 필터에 적용할 알파값 0.32 0.31 0.29

// 목표값
// 1600
// 1700
// 1850


// Servo range
#define _DUTY_MIN 1500 // [2952] 서보의 최소 각도값 550 1500 1300  1500     <1600>
#define _DUTY_NEU 1700 // [2952] 서보의 중간 각도값
#define _DUTY_MAX 2100 // [1691] 서보의 최대 각도  2400  1950 2100  2100    <1850>

// Servo speed control
#define _SERVO_ANGLE 30  //[2967] 서보 각도 설정
#define _SERVO_SPEED 420  //[2959] 서보의 속도 설정 400 410

// Event periods
#define _INTERVAL_DIST 20  //[2959] 센서의 거리측정 인터벌값 20
#define _INTERVAL_SERVO 37 //[2967] 서보 INTERVAL값 설정 37
#define _INTERVAL_SERIAL 100  //[2959] 시리얼 모니터/플로터의 인터벌값 설정


// PID parameters
#define _KP 0.99 // 비례이득 0.99 1.34
#define _KD 20 // 미분 이득 30 20 10 15 13 14 13.3 12.1 11 10.1 10.2 25 36 34 30 27.5 80 50

// [2964] 실제 거리에 따라서 센서가 읽는 값
#define r1 104  // 100mm  104
#define r2 134  // 150mm  131
#define r3 180  // 200mm  180
#define r4 212  // 250mm  211 212
#define r5 230  // 300mm  226
#define r6 241  // 350mm  238
#define r7 260  // 400mm  250


//////////////////////
// global variables //
//////////////////////

// Servo instance
Servo myservo;  // [2972] 서보 정의

// Distance sensor
int a, b, c, d, e, f, g; // raw dist
float dist_target; // location to send the ball
float dist_raw, dist_ema;    // [2961] dist_raw : 적외선센서로 얻은 거리를 저장하는 변수
                             // [2961] dist_ema : 거리를 ema필터링을 한 값을 저장하는 변수
float alpha;    // [2959] ema의 알파값을 저장할 변수


// Event periods
unsigned long last_sampling_time_dist, last_sampling_time_servo, last_sampling_time_serial;// [2957] last_sampling_time_dist : 거리센서 측정 주기
                          // [2957] last_sampling_time_servo : 서보 위치 갱신 주기
                          // [2957] last_sampling_time_serial : 제어 상태 시리얼 출력 주기
bool event_dist, event_servo, event_serial; // [2957] 각각의 주기에 도달했는지를 불리언 값으로 저장하는 변수

// Servo speed control
float duty_chg_per_interval; // [2952] 한 주기당 변화할 서보 활동량을 정의
float duty_target, duty_curr; // [2961] 목표위치, 서보에 입력할 위치

// PID variables
float error_curr, error_prev, control, kp, kd, pterm, dterm, iterm;

// 거리 구하는 함수

float calcDistance(short value) { // return value unit: mm
  float val, cali;
  float volt = float(analogRead(PIN_IR));
  val = ((6762.0 / (volt - 9.0)) - 4.0) * 10.0;

  // distance <= 150mm
  if (val <= r2) {
   cali = 100 + (150 - 100) / (r2 - r1) * (val - r1);
  }

  // 150mm < distance <= 200mm
  else if (val > r2 && val <= r3) {
    cali = 150 + (200 - 150) / (r3 - r2) * (val - r2);
  }

  // 200mm < distance <= 250mm
  else if (val > r3 && val <= r4) {
    cali = 200 + (250 - 200) / (r4 - r3) * (val - r3);
  }

  // 250mm < distance <= 300mm
  else if (val > r4 && val <= r5) {
    cali = 250 + (300 - 250) / (r5 - r4) * (val - r4);
  }

  // 300mm < distance <= 350mm
  else if (val > r5 && val <= r6) {
    cali = 300 + (350 - 300) / (r6 - r5) * (val - r5);
  }

  // 350mm < distance
  else {
    cali = 350 + (400 - 350) / (r7 - r6) * (val - r6);



 }

  return cali;
}

MedianFilter<calcDistance> filter;


void setup() {
// initialize GPIO pins for LED and attach servo 
pinMode(PIN_LED, OUTPUT); // [2952] LED를 GPIO 9번 포트에 연결
                          // [2957] LED를 출력 모드로 설정
myservo.attach(PIN_SERVO); // [2952] 서보 모터를 GPIO 10번 포트에 연결

// initialize filter
filter.init();


// initialize global variables
alpha = _DIST_ALPHA;   // [2959] ema의 알파값 초기화
dist_ema = 0;  // ema 초기화
duty_curr = _DUTY_NEU;// duty_curr을 중간값으로 초기화

// move servo to neutral position
myservo.writeMicroseconds(_DUTY_NEU);
// [2952] 서보 모터를 중간 위치에 지정


// initialize serial port
Serial.begin(115200); // [2952] 시리얼 포트를 115200의 속도로 연결

// P_control constant
kp = _KP; // 비례이득 저장
// D_control constant
kd = _KD; // 미분이득 저장


// convert angle speed into duty change per interval
duty_chg_per_interval = float((_DUTY_MAX - _DUTY_MIN)) * (_SERVO_SPEED / 180.0) * (_INTERVAL_SERVO / 1000.0); // [2959] 한 주기마다 이동할 양(180.0, 1000.0은 실수타입이기 때문에 나눗셈의 결과가 실수타입으로 리턴)
// [2974] INTERVAL -> _INTERVAL_SERVO 로 수정
// [2974] 이벤트 변수 초기화
last_sampling_time_dist = 0; // [2974] 마지막 거리 측정 시간 초기화
last_sampling_time_servo = 0; // [2974] 마지막 서보 업데이트 시간 초기화
last_sampling_time_serial = 0; // [2974] 마지막 출력 시간 초기화
event_dist = event_servo = event_serial = false; // [2974] 각 이벤트 변수 false로 초기화
}
  

void loop() {
/////////////////////
// Event generator //
/////////////////////

  unsigned long time_curr = millis();  // [2964] event 발생 조건 설정
  if(time_curr >= last_sampling_time_dist + _INTERVAL_DIST) {
    last_sampling_time_dist += _INTERVAL_DIST;
    event_dist = true; // [2957] 거리 측정 주기에 도달했다는 이벤트 발생
  }
  if(time_curr >= last_sampling_time_servo + _INTERVAL_SERVO) {
    last_sampling_time_servo += _INTERVAL_SERVO;
    event_servo = true; // [2957] 서보모터 제어 주기에 도달했다는 이벤트 발생
  }
  if(time_curr >= last_sampling_time_serial + _INTERVAL_SERIAL) {
    last_sampling_time_serial += _INTERVAL_SERIAL;
    event_serial = true; // [2957] 출력주기에 도달했다는 이벤트 발생
  }

////////////////////
// Event handlers //
////////////////////

  // get a distance reading from the distance sensor
  if(event_dist) { 
     event_dist = false;
//      dist_raw = ir_distance_filtered() * 10.0;   // [2959] dist_raw에 필터링된 측정값 저장
//      if (dist_ema == 0){                  // [2959] 맨 처음
//        dist_ema = dist_raw;               // [2959] 맨 처음 ema값 = 필터링된 측정값
//      }                                    // [2963] dist_ema를 dist_raw로 초기화
//      else{
//        dist_ema = alpha * dist_raw + (1-alpha) * dist_ema;   // [2959] ema 구현
//      }  
        if(filter.ready()){
          float dist_raw = filter.read();

          if (dist_ema == 0){
            dist_ema = dist_raw;
          }else{
            dist_ema = alpha * dist_raw + (1 - alpha) * dist_ema;
          }
        }
    
  // PID control logic
  
    error_curr = 255.0 - dist_ema;
     pterm = kp * error_curr;
    dterm = kd * (error_curr - error_prev);
    control = dterm + pterm;

  // duty_target = f(duty_neutral, control)
    if(error_curr > 0) {
      duty_target = 1700 + (1.5 * control);
    }else{
      duty_target = 1700 + control;
    }

  

  // keep duty_target value within the range of [_DUTY_MIN, _DUTY_MAX]
    if(duty_target < _DUTY_MIN) duty_target = _DUTY_MIN;
    else if (duty_target > _DUTY_MAX) duty_target = _DUTY_MAX;

  // update error_prev
     error_prev = error_curr;
  
  }
  
  if(event_servo) {
    event_servo = false; // [2974] 서보 이벤트 실행 후, 다음 주기를 기다리기 위해 이벤트 종료
    // adjust duty_curr toward duty_target by duty_chg_per_interval
    if(duty_target > duty_curr) {  // [2964] 현재 서보 각도 읽기
      duty_curr += duty_chg_per_interval; // [2961] duty_curr은 주기마다 duty_chg_per_interval만큼 증가
      if(duty_curr > duty_target) {duty_curr = duty_target;} // [2956] duty_target 지나쳤을 경우, duty_target에 도달한 것으로 duty_curr값 재설정
    }
    else {
      duty_curr -= duty_chg_per_interval;  // [2961] duty_curr은 주기마다 duty_chg_per_interval만큼 감소
      if (duty_curr < duty_target) {duty_curr = duty_target;} // [2956] duty_target 지나쳤을 경우, duty_target에 도달한 것으로 duty_curr값 재설정
    }
    // update servo position
    myservo.writeMicroseconds(duty_curr); // [2964] 서보 움직임 조절
  }
  
  if(event_serial) {
    event_serial = false; // [2974] 출력 이벤트 실행 후, 다음 주기까지 이벤트 종료
//    Serial.print("Min:100,Low:410,dist:");
//    Serial.print(dist_raw); // [2957] 적외선 센서로부터 받은 값 출력
//                            // [2959] 적외선 센서로부터 받은 값을 보정한 값 출력
//    Serial.print(",pterm:");
//    Serial.print(pterm);    // [2961] 비례제어량 출력
//    Serial.print(",duty_target:"); 
//    Serial.print(duty_target); // [2957] 목표로 하는 거리 출력
//    Serial.print(",duty_curr:"); 
//    Serial.print(duty_curr); // [2957] 서보모터에 입력한 값 출력
//    Serial.print("duty_chg_per_interval :");
//    Serial.print(duty_chg_per_interval);
//    Serial.println(",High:310,Max:2000");
      Serial.print("dist_IR:");
      Serial.print(dist_ema);
      Serial.print(", pterm:");
      Serial.print(map(pterm, -1000, 1000, 510, 610));
      Serial.print(",dterm:");
      Serial.print(map(dterm, -1000, 1000, 510, 610));
      Serial.print(",duty_target:");
      Serial.print(map(duty_target, 1000, 2000, 410, 510));
      Serial.print(", duty_curr:");
      Serial.print(map(duty_curr, 1000, 2000, 410, 510));
      Serial.println(",Min:100, Low:200, dist_target:255, High:310, Max:410");
  }
}
//float ir_distance(void){ // return value unit: mm
//                         // [2959] 센서가 측정한 거리를 리턴해주는 함수
//  float val, dist_cali;
//  float volt = float(analogRead(PIN_IR));
//  val = ((6762.0/(volt-9.0))-4.0) * 10.0;
//
//  a = r1;
//  b = r2;
//  c = r3;
//  d = r4;
//  e = r5;
//  f = r6;
//  g = r7;
//
//  
//  if (val <= r2){
//    dist_cali = 100 + (150 - 100) / (b - a) * (val - a); // 15cm 이하 보정
//  }
//  
//
//  else if(val > r2 && val <= r3){
//    dist_cali = 150 + (200 - 150) / (c - b) * (val - b); // 15cm 이상 20cm 이하 보정
//  }
//  
//  
//  else if(val > r3 && val <= r4){
//    dist_cali = 200 + (250 - 200) / (d - c) * (val - c); // 20cm 이상 25cm 이하 보정
//  }
//  
//  
//  else if(val > r4 && val <= r5){
//    dist_cali = 250 + (300 - 250) / (e - d) * (val - d); // 25cm 이상 30cm 이하 보정
//  }
//
//  
//  else if(val > r5 && val <= r6){
//    dist_cali = 300 + (350 - 300) / (f - e) * (val - e); // 30cm 이상 35cm 이하 보정
//  }
//
//
//  else{
//    dist_cali = 350 + (400 - 350) / (g - f) * (val - f); // 35cm 이상 40cm 이하 보정
//  }
//
//  return dist_cali;
//
//  //return 100 + 300.0 / (b - a) * (val - a);  // [2964] 센서가 읽은 거리를 이용한 실제
//      // 거리 반환
//}                                              // [2959] 윗줄 ir_distance_filtered로 이동
//
//float ir_distance_filtered(void){ // return value unit: mm
//  
//}