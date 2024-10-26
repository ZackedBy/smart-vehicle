#include <LiquidCrystal.h>

//JoyStick Variables
#define joy_key 18
volatile boolean button_pressed = false;
volatile int button_count = 0;
const unsigned long debounce_time = 50;
unsigned long last_button_press_time = 0;

//Motor Variables
#define Motor_L_dir_pin 7
#define Motor_R_dir_pin 8
#define Motor_L_pwm_pin 9
#define Motor_R_pwm_pin 10
const byte Motor_forward = 0;
const byte Motor_backward = 1;
//Encoder Variables
#define ENC_A_R 2   //distance sensing
#define ENC_A_L 3
//#define ENC_B_R 11  //direction sensing
//#define ENC_B_L 12
volatile int pulse_count_R = 0;
volatile int pulse_count_L = 0;
const float wheel_dia_m = 0.037; //3cm for wheel, 2x0.35cm for thread
const float wheel_circ = PI * wheel_dia_m;
const int pulse_per_revolution = 210; //Find actual value
const float pulse_per_cm = (float)pulse_per_revolution / (wheel_circ * 100);
float distance_travelled = 0;
const float target_distance = 100.0;

//LCD Variables
#define rs 32
#define en 30
#define d4 28
#define d5 26
#define d6 24
#define d7 22
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned long prev_millis_lcd = 0;
const int lcd_interval = 300;

void setup() {
  lcd.begin(20,4);
  attachInterrupt(digitalPinToInterrupt(joy_key), joystick_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_R), encoder_pulse_R, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_L), encoder_pulse_L, RISING);
  Serial.begin(9600);
}

void loop() {
  if (button_pressed) {
    button_pressed = false;
    travel_distance();
  }
  update_lcd();
}

void travel_distance(){
  distance_travelled = 0;
  pulse_count_R = 0;
  pulse_count_L = 0;

  move_motor(Motor_forward, Motor_forward, 255, 255);
  while (true){
//    distance_travelled = (
//        ((pulse_count_R * wheel_circ) / pulse_per_revolution) + 
//        ((pulse_count_L * wheel_circ) / pulse_per_revolution)
//    ) / 2 * 100; // Average distance in cm
    
    distance_travelled = ( (pulse_count_L + pulse_count_R) / 2.0) / pulse_per_cm;

    update_lcd();
    
    if (distance_travelled >= target_distance) {
      break;
    }
  }
  move_motor(0, 0, 0, 0);
}

void move_motor(byte left, byte right, int l_speed, int r_speed){
  //Direction
  digitalWrite(Motor_L_dir_pin, left);
  digitalWrite(Motor_R_dir_pin, right);
  //Speed
  analogWrite(Motor_L_pwm_pin, l_speed); 
  analogWrite(Motor_R_pwm_pin, r_speed); 
}

//Interrupt methods
void encoder_pulse_R(){
  pulse_count_R++;
}
void encoder_pulse_L(){
  pulse_count_L++;
}
//Probably get rid of timer
void joystick_interrupt() {
  unsigned long current_time = millis();
  if (current_time - last_button_press_time > debounce_time) {
    button_count++;
    button_pressed = true;
    last_button_press_time = current_time;
  }
}

void update_lcd(){
  unsigned long current_millis_lcd = millis();
  //Non-disruptive delay
  if (current_millis_lcd - prev_millis_lcd >= lcd_interval) {
    prev_millis_lcd = current_millis_lcd;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Pulse R: " + String(pulse_count_R));
    lcd.setCursor(0,1);
    lcd.print("Pulse L: " + String(pulse_count_L));
    lcd.setCursor(0,2);
    lcd.print("Dist.: " + String(distance_travelled) + "cm");
    lcd.setCursor(0,3);
    lcd.print("P/cm.: " + String(pulse_per_cm));
  }
}
