#include <LiquidCrystal.h>

//JoyStick Variables
#define joy_x                 A2
#define joy_y                 A1
#define joy_key               18
int joy_x_value = 0;
int joy_y_value = 0;
double x_volt = 0;
double y_volt = 0;
//JoyStick Press Variables
volatile int button_count = 0;
volatile boolean button_pressed = false;
volatile unsigned long prev_interrupt_time = 0;
const int debounce_time = 50;

//Motor Variables
#define Motor_L_dir_pin       7
#define Motor_R_dir_pin       8
#define Motor_L_pwm_pin       9
#define Motor_R_pwm_pin       10
const int Motor_forward = 0;
const int Motor_backward = 1;
int pwm_R=100;
int pwm_L=100; 
boolean motor_test_active = false;

//LCD Variables
const int rs = 32, en = 30, d4 = 28, d5 = 26, d6 = 24, d7 = 22;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned long prev_millis_lcd = 0;
const int lcd_interval = 500;

void setup() {
  lcd.begin(20,4);
  attachInterrupt(digitalPinToInterrupt(joy_key), joystick_interrupt, FALLING);
  Serial.begin(9600);
}

void loop(){
  joy_x_value = analogRead(joy_x);
  joy_y_value = analogRead(joy_y);
  x_volt = joy_x_value / 204.6;
  y_volt = joy_y_value / 204.6;

  lcd_display();
}

void lcd_display(){
  unsigned long current_millis_lcd = millis();
  //Non-disruptive delay
  if (current_millis_lcd - prev_millis_lcd >= lcd_interval) {
    prev_millis_lcd = current_millis_lcd;
    lcd.clear();
    if(button_pressed) {
      lcd.setCursor(0,0);
      lcd.print("Push Counter:");
      lcd.setCursor(0,1);
      lcd.print(String(button_count));
      if (!motor_test_active){ //only run it when not moving
         motor_test(); 
      }
    } else {
      lcd.setCursor(0,0);
      lcd.print("x:" + String(joy_x_value) + " " + String(x_volt) + " V");
      lcd.setCursor(0,1);
      lcd.print("y:" + String(joy_y_value) + " " + String(y_volt) + " V");
      Serial.println(joy_y_value);
    }
  }
}

void motor_test() {
  motor_test_active = true;
  //Spee (reduce by 10 each button press)
  analogWrite(Motor_L_pwm_pin,pwm_L);
  analogWrite(Motor_R_pwm_pin,pwm_R);
  //Direction
  digitalWrite(Motor_R_dir_pin,Motor_backward);  
  digitalWrite(Motor_L_dir_pin,Motor_backward);
  delay(2000);
  digitalWrite(Motor_R_dir_pin,Motor_forward);  
  digitalWrite(Motor_L_dir_pin,Motor_forward); 
  delay(2000);
  //Set speed to 0
  analogWrite(Motor_L_pwm_pin,0);
  analogWrite(Motor_R_pwm_pin,0);
  //Reduce speed
  pwm_L -= 10;
  pwm_R -= 10; 
  //reset to 100 if too low
  if (pwm_R <= 10 || pwm_L <= 10) {
    pwm_R = 100;
    pwm_L = 100;
  } 
  motor_test_active = false;   
}

void joystick_interrupt() {
  button_pressed = !button_pressed;
  button_count++;
}
