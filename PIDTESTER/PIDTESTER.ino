#include <Servo.h> // 서보 헤더파일 포함


/////////////////////////////
// Configurable parameters //
/////////////////////////////


// Arduino pin assignment
#define PIN_LED 9 // LED를 아두이노 GPIO 9번 핀에 연결
#define PIN_SERVO 10 // servo moter를 아두이노 GPIO 10번 핀에 연결
#define PIN_IR A0 // 적외선센서를 아두이노 A0핀에 연결

#include "medianfilter.h" // 중위수필터 적용

// Framework setting
#define _DIST_TARGET 255.0 // 탁구공 위치까지의 거리를 255로 고정
#define _DIST_MIN 100 // 거리 센서가 인식 가능하게 설정한 최소 거리
#define _DIST_MAX 410 // 거리 센서가 인식 가능하게 설정한 최대 거리


// Distance sensor
#define _DIST_ALPHA 0.32  // ema 필터 알파값


// Servo range
#define _DUTY_MIN 1100 // 서보의 최소 각도값  1300    1150  900              1250   
#define _DUTY_NEU 1500 // 서보의 중간 각도값  1700    1400                   1500
#define _DUTY_MAX 1900 // 서보의 최대 각도     2000   1600  2100             1700
 
// Servo speed control
#define _SERVO_ANGLE 60  // 서보 각도 설정
#define _SERVO_SPEED 250  // 서보의 속도 설정 250 180


// Event periods
#define _INTERVAL_DIST 20  // 센서의 거리측정 인터벌값
#define _INTERVAL_SERVO 20 // 서보 INTERVAL값 설정
#define _INTERVAL_SERIAL 100  // 시리얼 모니터/플로터의 인터벌값 설정


// PID parameters
#define _KP 1.5 // 비례이득 1.5 1.7  1.5
#define _KD 31 // 미분이득  35.7 33.8 41 35.0 38.0 53 52 29 30 32 33 34
//#define _KD1 50.0  // 왼쪽 34.0  38
//#define _KD2 45.0  // 36.0       41
#define _KI 0.02 // 적분이득 0.005 0.013   0.02 0.05 0.03 0.02 0.03



// 10cm 20cm 25cm 30cm 40cm 일때 읽는 값
#define A  76     // 78      71            
#define B  169    // 297     183 170 172
#define B_25 204  // 199          204
#define C  222    // 222
#define D  289    //283  

// 거리보정
//#define A 96
//#define B 129
//#define C 169
//#define D 198
//#define E 221
//#define F 247
//#define G 278

//////////////////////
// global variables //
//////////////////////

// Servo instance
Servo myservo;        // 서보를 정의함

// Distance sensor
float dist_target;  // 목표 위치
float dist_raw, dist_ema;  // raw 적외선 읽은 값 저장
                           // ema ema 필터링 적용 값 저장
                          
float alpha; // ema의 알파값을 저장할 변수
float cali; // 거리 보정값 변수

// Event periods
unsigned long last_sampling_time_dist, last_sampling_time_servo, last_sampling_time_serial;// [2957] last_sampling_time_dist : 거리센서 측정 주기
// last_sampling_time_servo : 서보 위치 갱신 주기
// last_sampling_time_serial : 제어 상태 시리얼 출력 주기
bool event_dist, event_servo, event_serial; // 각각의 주기에 도달했는지를 불리언 값으로 저장하는 변수

// Servo speed control
int duty_chg_per_interval; // 한 주기당 변화할 서보 활동량을 정의
int duty_target, duty_curr; // 서보 목표 위치


// PID variables
float error_curr, error_prev, control, pterm, dterm, iterm;

// 센서값 읽고 실제 거리로 변환한 값 리턴하는 함수
float calcDistance(short value){ // return value unit: mm
  float volt = float(analogRead(PIN_IR));
  float val = ((6762.0/(volt-9.0))-4.0) * 10.0;

  if(val < A){
    return 100;
  }
  else if (val < B){
    return 100 + 100.0 * (val - A) / (B - A);
  }
//  else if (val < B_25){
//    return 200 + 50.0 * (val - B) / (B_25 - B);
//  }
//  else if (val < C){
//    return 250 + 50.0 * (val - B_25) / (C - B_25);
//  }
  else if (val < C){
    return 200 + 100.0 * (val - B) / (C - B);
  }
  else if (val < D){
    return 300 + 100.0 * (val - C) / (D - C);
  }
  else{
    return 400;
  }

}

MedianFilter<calcDistance> filter;


void setup() {

// initialize GPIO pins for LED and attach servo 
pinMode(PIN_LED, OUTPUT); // LED를 GPIO 9번 포트에 연결
                          // LED를 출력 모드로 설정
myservo.attach(PIN_SERVO); // 서보 모터를 GPIO 10번 포트에 연결

iterm = 0; // 0으로 초기화


// initialize global variables
alpha = _DIST_ALPHA; // ema의 알파값 초기화
dist_ema = 0; // dist_ema 초기화
duty_target = duty_curr = _DUTY_NEU; // duty_target, duty_curr 초기화
error_prev = 0;


// move servo to neutral position
myservo.writeMicroseconds(_DUTY_NEU); // 서보 모터를 중간 위치에 지정


// initialize serial port
Serial.begin(115200); // 시리얼 포트를 115200의 속도로 연결

filter.init();

// convert angle speed into duty change per interval.
duty_chg_per_interval = (_DUTY_MAX - _DUTY_MIN) * (_SERVO_SPEED / 180.0) * (_INTERVAL_SERVO / 1000.0); 
// 한 주기마다 이동할 양(180.0, 1000.0은 실수타입이기 때문에 나눗셈의 결과가 실수타입으로 리턴)

// 이벤트 변수 초기화
last_sampling_time_dist = 0; // 마지막 거리 측정 시간 초기화
last_sampling_time_servo = 0; // 마지막 서보 업데이트 시간 초기화
last_sampling_time_serial = 0; // 마지막 출력 시간 초기화
event_dist = event_servo = event_serial = false; // 각 이벤트 변수 false로 초기화
}
  

void loop() {
/////////////////////
// Event generator //
/////////////////////
  unsigned long time_curr = millis();  // event 발생 조건 설정
  if(time_curr >= last_sampling_time_dist + _INTERVAL_DIST) {
    last_sampling_time_dist += _INTERVAL_DIST;
    event_dist = true; // 거리 측정 주기에 도달했다는 이벤트 발생
  }
  if(time_curr >= last_sampling_time_servo + _INTERVAL_SERVO) {
    last_sampling_time_servo += _INTERVAL_SERVO;
    event_servo = true; // 서보모터 제어 주기에 도달했다는 이벤트 발생
  }
  if(time_curr >= last_sampling_time_serial + _INTERVAL_SERIAL) {
    last_sampling_time_serial += _INTERVAL_SERIAL;
    event_serial = true; // 출력주기에 도달했다는 이벤트 발생
  }


////////////////////
// Event handlers //
////////////////////

  // get a distance reading from the distance sensor         
      if(event_dist) { 
        event_dist = false;       
        if(filter.ready()){ // 필터가 값을 읽을 준비가 되었는지 확인
          dist_raw = filter.read();   // dist_raw에 필터링된 측정값 저장
          
        if (dist_ema == 0){                  
            dist_ema = dist_raw;               // ema = 0? > dist_raw로 초기화
          }                                    
        else{
            dist_ema = alpha * dist_raw + (1-alpha) * dist_ema; // ema 필터링 적용
          }     

    // PID control logic
    error_curr = _DIST_TARGET - dist_ema;
    dterm = _KD * (error_curr - error_prev);
    pterm = error_curr * _KP;
    iterm += _KI * (error_curr);
    if(error_curr > 0){
      dterm = _KD * (error_curr - error_prev);
      control= (pterm + dterm + iterm);  // 0.75  0.7 0.6  0.4 0.6 0.73 1.03
    }
    else{
    dterm = _KD * (error_curr - error_prev);
    control=   1.25 * (pterm + dterm + iterm);    // 0.6  0.5 0.3 0.4  0.5 0.6 0.8
    }
    
    ;
 
  // duty_target = f(_DUTY_NEU, control)
    duty_target = 1500 + control;
    
  // keep duty_target value within the range of [_DUTY_MIN, _DUTY_MAX]
  //duty_target이 _DUTY_MIN, _DUTY_MAX의 범위 안에 있도록 제어
    if(duty_target < _DUTY_MIN) { duty_target = _DUTY_MIN; } 
    else if(duty_target > _DUTY_MAX) { duty_target = _DUTY_MAX; }

    //update error_prev
    error_prev = error_curr;
    }
}
  
  if(event_servo) {
    event_servo = false; // 서보 이벤트 실행 후, 다음 주기를 기다리기 위해 이벤트 종료
    
    // adjust duty_curr toward duty_target by duty_chg_per_interval
    if(duty_target > duty_curr) {  // 현재 서보 각도 읽기
      duty_curr += duty_chg_per_interval; // duty_curr은 주기마다 duty_chg_per_interval만큼 증가
      if(duty_curr > duty_target) {duty_curr = duty_target;} // duty_target 지나쳤을 경우, duty_target에 도달한 것으로 duty_curr값 재설정
    }
    else {
      duty_curr -= duty_chg_per_interval;  // duty_curr은 주기마다 duty_chg_per_interval만큼 감소
      if (duty_curr < duty_target) {duty_curr = duty_target;} // duty_target 지나쳤을 경우, duty_target에 도달한 것으로 duty_curr값 재설정
    }
    
    // update servo position
    myservo.writeMicroseconds(duty_curr);  // 서보 움직임 조절
  }
  
  if(event_serial) {
    event_serial = false; // 출력 이벤트 실행 후, 다음 주기까지 이벤트 종료
    Serial.print("IR:");
    Serial.print(dist_ema);
    Serial.print(",T:");
    Serial.print(_DIST_TARGET);
    Serial.print(",P:");
    Serial.print(map(pterm, -1000, 1000, 510, 610));
    Serial.print(",D:");
    Serial.print(map(dterm, -1000, 1000, 510, 610));
    Serial.print(",I:");
    Serial.print(map(iterm, -1000, 1000, 510, 610));
    Serial.print(",DTT:");
    Serial.print(map(duty_target, 1000, 2000, 410, 510));
    Serial.print(",DTC:");
    Serial.print(map(duty_curr, 1000, 2000, 410, 510));
    Serial.println(",-G:245,+G:265,m:0,M:800");
  }
}


// return 100.0 + 300.0 / (B - A) * (val - A);  // 실제 거리 
//  // distance <= 150mm
//  if (val <= B) {
//   cali = 100 + (150 - 100) / (B - A) * (val - A);
//  }
//
//  // 150mm < distance <= 200mm
//  else if (val > B && val <= C) {
//    cali = 150 + (200 - 150) / (C - B) * (val - B);
//  }
//
//  // 200mm < distance <= 250mm
//  else if (val > C && val <= D) {
//    cali = 200 + (250 - 200) / (D - C) * (val - C);
//  }
//
//  // 250mm < distance <= 300mm
//  else if (val > D && val <= E) {
//    cali = 250 + (300 - 250) / (E - D) * (val - D);
//  }
//
//  // 300mm < distance <= 350mm
//  else if (val > E && val <= F) {
//    cali = 300 + (350 - 300) / (F - E) * (val - E);
//  }
//
//  // 350mm < distance
//  else {
//    cali = 350 + (400 - 350) / (G - F) * (val - F);
//  }
//  return cali;
