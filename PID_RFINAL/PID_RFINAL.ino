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
#define _DIST_MIN 100 // 측정 최소 거리
#define _DIST_MAX 410 // 측정 최대 거리


// Distance sensor
#define _DIST_ALPHA 0.2   // ema 필터 알파값


// Servo range
#define _DUTY_MIN 1300 // 서보의 최소 각도값 550     1600
#define _DUTY_NEU 1700 // 서보의 중간 각도값         1700
#define _DUTY_MAX 1900 // 서보의 최대 각도           1850

// Servo speed control
#define _SERVO_ANGLE 60  // 서보 각도 설정
#define _SERVO_SPEED 240  // 서보의 속도 설정


// Event periods
#define _INTERVAL_DIST 20  // 센서의 거리측정 인터벌값
#define _INTERVAL_SERVO 20 // 서보 INTERVAL값 설정
#define _INTERVAL_SERIAL 100  // 시리얼 모니터/플로터의 인터벌값 설정


// PID parameters
#define _KP 1.7 // 비례이득
#define _KD 30.0 // 미분이득  33.0 
#define _KI 0.009 // 적분이득 0.007 



// 10cm 40cm 
#define A  104    // 104               
#define B  297    // 297                  

//////////////////////
// global variables //
//////////////////////

// Servo instance
Servo myservo;        // 서보 정의

// Distance sensor
float dist_target; // location to send the ball
float dist_cali, dist_ema, dist_raw; // dist_cali : 보정값
                          // dist_ema : 거리를 ema필터링을 한 값을 저장하는 변수
                          // dist_raw : 중위수 필터 적용
int a, b, c, d, e, f, g; // 구간별 거리 보정
float alpha; // ema의 알파값을 저장할 변수

// Event periods
unsigned long last_sampling_time_dist, last_sampling_time_servo, last_sampling_time_serial;// [2957] last_sampling_time_dist : 거리센서 측정 주기
// last_sampling_time_servo : 서보 위치 갱신 주기
// last_sampling_time_serial : 제어 상태 시리얼 출력 주기
bool event_dist, event_servo, event_serial; // 각각의 주기에 도달했는지를 불리언 값으로 저장하는 변수

// Servo speed control
int duty_chg_per_interval; // 한 주기당 변화할 서보 활동량을 정의
int duty_target, duty_curr; // 목표위치, 서보에 입력할 위치


// PID variables
float error_curr, error_prev, control, pterm, dterm, iterm;

// 센서값 읽고 실제 거리로 변환한 값 리턴하는 함수
float calcDistance(short value){ // return value unit: mm
  float volt = float(analogRead(PIN_IR));
  float val = ((6762.0/(volt-9.0))-4.0) * 10.0; // *10 : cm -> mm로 변환
  return 100.0 + 300.0 / (B - A) * (val - A);

//  // distance <= 150mm
//  if (val <= b){
//    dist_cali = 100 + (150 - 100) / (b - a) * (val - a);
//  }
//  
//  // 150mm < distance <= 200mm
//  else if(val > b && val <= c){
//    dist_cali = 150 + (200 - 150) / (c - b) * (val - b);
//  }
//  
//  // 200mm < distance <= 250mm
//  else if(val > c && val <= d){
//    dist_cali = 200 + (250 - 200) / (d - c) * (val - c);
//  }
//  
//  // 250mm < distance <= 300mm
//  else if(val > d && val <= e){
//    dist_cali = 250 + (300 - 250) / (e - d) * (val - d);
//  }
//
//  // 300mm < distance <= 350mm
//  else if(val > e && val <= f){
//    dist_cali = 300 + (350 - 300) / (f - e) * (val - e);
//  }
//
//  // 350mm < distance
//  else{
//    dist_cali = 350 + (400 - 350) / (g - f) * (val - f);
//  }
//  
//  return dist_cali;
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

// 구간별 거리 보정
a = 77;  // 100
b = 127;  // 150
c = 170;  // 200
d = 190;  // 250                
e = 218;  // 300
f = 242;  // 350
g = 283;  // 400

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
  if(event_dist){
    event_dist = false;
    
      if(filter.ready()){ 
          dist_raw = filter.read();   // 중위수 필터링 적용 결과 저장
          
        if (dist_ema == 0){                  
            dist_ema = dist_raw;               // dist_ema = 0? >> dist_raw로 초기화 시킨다
          }                                    
        else{
            dist_ema = alpha * dist_raw + (1-alpha) * dist_ema; // ema필터링 적용
          }     

  // get a distance reading from the distance sensor             
           

    // PID control logic
    error_curr = _DIST_TARGET - dist_ema;
    dterm = _KD * (error_curr - error_prev);
    pterm = error_curr * _KP;
    iterm += _KI * error_curr;
    if(pterm > 0){
      control= 0.7 * (pterm + dterm);
    }
    else{
      control= pterm + dterm; 
    }
    ;
 
  // duty_target = f(_DUTY_NEU, control)
    duty_target = _DUTY_NEU + control;
    
  // keep duty_target value within the range of [_DUTY_MIN, _DUTY_MAX]
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
