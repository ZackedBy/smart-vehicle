/*
 * #define used ONLY for pin declarations
 */

#include <LiquidCrystal.h>

//JoyStick Variables
#define joy_x A2
#define joy_y A1
#define joy_key 18
int joy_x_value = 0;
int joy_y_value = 0;
double x_volt = 0;
double y_volt = 0;
//JoyStick Press Variables
#define led_pin 13
volatile int button_count = 0;
volatile boolean button_pressed = false;

//Motor Variables
#define Motor_L_dir_pin 7
#define Motor_R_dir_pin 8
#define Motor_L_pwm_pin 9
#define Motor_R_pwm_pin 10
const byte Motor_forward = 1;
const byte Motor_backward = 0;
const int stationary_x = 511; 
int motor_speed_x = 0;
const int stationary_y = 519;
int motor_speed_y = 0;
//Encoder Variables
#define ENC_A_R 2  
#define ENC_A_L 3
#define ENC_B_R 11
#define ENC_B_L 12
volatile int pulse_count_R = 0;
volatile int pulse_count_L = 0;
//const float wheel_dia_m = 0.038; //Find actual value
//const float wheel_circ = PI * wheel_dia_m;
//const int pulse_per_revolution = 360; //Find actual value
//const float pulse_per_cm = pulse_per_revolution / (wheel_circ * 100);
//float distance_travelled = 0;


//LCD Variables
#define rs 32
#define en 30
#define d4 28
#define d5 26
#define d6 24
#define d7 22
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned long prev_millis_lcd = 0;
const int lcd_interval = 500;

void setup() {
  lcd.begin(20,4);
  pinMode(led_pin,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(joy_key), joystick_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_R), encoder_pulse_R, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_L), encoder_pulse_L, RISING);
  Serial.begin(9600);
}

void loop(){
  joy_x_value = analogRead(joy_x);
  joy_y_value = analogRead(joy_y);
  x_volt = joy_x_value / 204.6;
  y_volt = joy_y_value / 204.6;
  //x_volt = joy_x_value * (5.0 / 1023.0);
  //y_volt = joy_y_value * (5.0 / 1023.0);


  joystick_pressed();
  lcd_display();
  motor_logic();
}

void lcd_display(){
  unsigned long current_millis_lcd = millis();
  //Non-disruptive delay
  if (current_millis_lcd - prev_millis_lcd >= lcd_interval) {
    prev_millis_lcd = current_millis_lcd;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("x:" + String(joy_x_value) + " " + String(x_volt) + " V");
    lcd.setCursor(0,1);
    lcd.print("y:" + String(joy_y_value) + " " + String(y_volt) + " V");
    lcd.setCursor(0,2);
    lcd.print("Pulse_R: " + String(pulse_count_R));
    lcd.setCursor(0,3);
    lcd.print("Pulse_L: " + String(pulse_count_L));
  }
}


//There must be a easier way...
void motor_logic() {
    // Speed Control
    motor_speed_x = (joy_x_value < stationary_x)
                  ? map(joy_x_value, 0, stationary_x, 255, 0)   // Forward (X-axis)
                  : map(joy_x_value, stationary_x, 1023, 0, 255); // Backward (X-axis)
                  
    motor_speed_y = (joy_y_value < stationary_y)
                  ? map(joy_y_value, 0, stationary_y, 255, 0)   // Right (Y-axis)
                  : map(joy_y_value, stationary_y, 1023, 0, 255); // Left (Y-axis)

    // Joystick dead zone (stationary region)
    bool x_in_deadzone = abs(joy_x_value - stationary_x) <= 10;
    bool y_in_deadzone = abs(joy_y_value - stationary_y) <= 10;

    // Determine movement based on joystick direction
    if (x_in_deadzone && joy_y_value > stationary_y) {                          // Move left
        move_motor(Motor_forward, Motor_backward, motor_speed_y, motor_speed_y);
    } else if (x_in_deadzone && joy_y_value < stationary_y) {                   // Move right
        move_motor(Motor_backward, Motor_forward, motor_speed_y, motor_speed_y);
    } else if (y_in_deadzone && joy_x_value < stationary_x) {                   // Move forward
        move_motor(Motor_forward, Motor_forward, motor_speed_x, motor_speed_x);
    } else if (y_in_deadzone && joy_x_value > stationary_x) {                   // Move backward
        move_motor(Motor_backward, Motor_backward, motor_speed_x, motor_speed_x);
    } else if (joy_x_value > stationary_x && joy_y_value > stationary_y) {      // Backward-left
        move_motor(Motor_backward, Motor_backward, motor_speed_x, 0);
    } else if (joy_x_value > stationary_x && joy_y_value < stationary_y) {      // Backward-right
        move_motor(Motor_backward, Motor_backward, 0, motor_speed_x);
    } else if (joy_x_value < stationary_x && joy_y_value > stationary_y) {      // Forward-left
        move_motor(Motor_forward, Motor_forward, motor_speed_x, 0);
    } else if (joy_x_value < stationary_x && joy_y_value < stationary_y) {      // Forward-right
        move_motor(Motor_forward, Motor_forward, 0, motor_speed_x);
    } else {                                                                    // Stationary (dead zone)
        move_motor(0, 0, 0, 0);
    }
}



void move_motor(byte left, byte right, int l_speed, int r_speed){
  //Direction
  digitalWrite(Motor_L_dir_pin, left);
  digitalWrite(Motor_R_dir_pin, right);
  //Speed
  analogWrite(Motor_L_pwm_pin, l_speed); 
  analogWrite(Motor_R_pwm_pin, r_speed); 
}

void joystick_pressed(){
  //Turn LED on/off with joystick press
  digitalWrite(led_pin, button_pressed ? HIGH : LOW);
}

void joystick_interrupt() {
  button_pressed = !button_pressed;
  button_count++;
  //Serial.println(String(button_count));
  pulse_count_L = 0;
  pulse_count_R = 0;
}
void encoder_pulse_R(){
  pulse_count_R++;
}
void encoder_pulse_L(){
  pulse_count_L++;
}
