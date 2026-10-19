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
#define led_pin 5
volatile int button_count = 0;
volatile boolean button_pressed = false;
volatile unsigned long prev_interrupt_time = 0;
const int debounce_time = 50;

//Motor Variables
#define Motor_L_dir_pin 7
#define Motor_R_dir_pin 8
#define Motor_L_pwm_pin 9
#define Motor_R_pwm_pin 10
const int Motor_forward = 0;
const int Motor_backward = 1;
const int x_min = 500; 
const int x_max = 520; 
const int stationary_x = 511; 
int motor_speed = 0;

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
  Serial.begin(9600);
}

void loop(){
  joy_x_value = analogRead(joy_x);
  joy_y_value = analogRead(joy_y);
  x_volt = joy_x_value / 204.6; //assuming 5V 
  y_volt = joy_y_value / 204.6; //assuming 5V

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
    Serial.println("X:" + String(joy_x_value) + ", Y:" + String(joy_y_value));
  }
}

void motor_logic(){
  //Speed Control
  motor_speed = (joy_x_value < stationary_x)
                ? map(joy_x_value, 0, stationary_x, 255, 0) //forward 
                : map(joy_x_value, stationary_x, 1023, 0, 255); //back
  //map = return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  //Direction Control
  if (joy_x_value > x_min && joy_x_value < x_max) { //not moving joystick left or right
    if (joy_y_value > 1000) {
      move_motor(Motor_backward, Motor_forward, 100);  //left turn
    } else if (joy_y_value < 20) {
      move_motor(Motor_forward, Motor_backward, 100);  //right turn
    } else {
      move_motor(0, 0, 0);  //stationary
    }
  } else if (joy_x_value < stationary_x) {
    move_motor(Motor_forward, Motor_forward, motor_speed);  //forward
  } else {
    move_motor(Motor_backward, Motor_backward, motor_speed);  //backward
  }
}

void move_motor(int left, int right, int m_speed){
  //Direction
  digitalWrite(Motor_L_dir_pin, left);
  digitalWrite(Motor_R_dir_pin, right);
  //Speed
  analogWrite(Motor_L_pwm_pin, m_speed); 
  analogWrite(Motor_R_pwm_pin, m_speed); 
}

void joystick_pressed(){
  //Turn LED on/off with joystick press
  digitalWrite(led_pin, button_pressed ? HIGH : LOW);
}

void joystick_interrupt() {
  volatile unsigned long interrupt_time = millis();
  //Non-disruptive delay [maybe not needed since low-pass filter (capacitor) was added]
  if (interrupt_time - prev_interrupt_time >= debounce_time){
    prev_interrupt_time = interrupt_time;
    button_pressed = !button_pressed;
    button_count++;
  }
}
