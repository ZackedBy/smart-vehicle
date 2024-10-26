#include <LiquidCrystal.h> 


//JoyStick Variables 
#define joy_x A2 
#define joy_y A1 
#define joy_key 18 
int joy_x_value = 0; 
int joy_y_value = 0; 
double x_volt = 0; 
double y_volt = 0; 
volatile int button_count = 1; 
volatile boolean button_pressed = false; 

//Motor Variables 
#define Motor_forward         1 
#define Motor_backward        0
#define Motor_L_dir_pin       7 
#define Motor_R_dir_pin       8 
#define Motor_L_pwm_pin       9 
#define Motor_R_pwm_pin       10 
int pwm_R=100;
int pwm_L=100;

const int rs = 32, en = 30, d4 = 28, d5 = 26, d6 = 24, d7 = 22; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); 

const int lcd_delay = 500;
unsigned long prev_millis_lcd = 0;

void setup() { 
  lcd.begin(20,4);  
  attachInterrupt(digitalPinToInterrupt(joy_key), joystickPressed, FALLING); 
  Serial.begin(9600); 
} 


void loop(){ 
  joy_x_value = analogRead(joy_x); 
  joy_y_value = analogRead(joy_y); 
  x_volt = joy_x_value / 204.6; 
  y_volt = joy_y_value / 204.6; 
  unsigned long current_millis_lcd = millis();
  if (current_millis_lcd - prev_millis_lcd >= lcd_delay){
    lcd.clear(); 
    prev_millis_lcd = current_millis_lcd;
    if(button_pressed) { 
      lcd.setCursor(0,0); 
      lcd.print("Push Counter:"); 
      lcd.setCursor(0,1); 
      lcd.print(String(button_count - 1));
    } else{ 
     lcd.setCursor(0,0); 
     lcd.print("x:" + String(joy_x_value) + " " + String(x_volt) + " V"); 
     lcd.setCursor(0,1); 
     lcd.print("y:" + String(joy_y_value) + " " + String(y_volt) + " V"); 
     Serial.println(joy_y_value); 
    } 
    if (button_count % 2 == 0) {
      pwm_R = 100, pwm_L = 100;
      motor_test_diy();
    }
  }

  if(joy_x_value > 800) { 
    digitalWrite(Motor_R_dir_pin, Motor_backward);   
    digitalWrite(Motor_L_dir_pin, Motor_backward); 
    analogWrite(Motor_L_pwm_pin,100); 
    analogWrite(Motor_R_pwm_pin,100); 
  } 
  else if(joy_x_value < 300) { 
    digitalWrite(Motor_R_dir_pin, Motor_forward);   
    digitalWrite(Motor_L_dir_pin, Motor_forward); 
    analogWrite(Motor_L_pwm_pin,100); 
    analogWrite(Motor_R_pwm_pin,100); 
  } 
  else if(joy_x_value > 300 && joy_x_value < 800){ 
    analogWrite(Motor_L_pwm_pin,0); 
    analogWrite(Motor_R_pwm_pin,0); 
  }   
} 




void motor_test_diy() {
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
  digitalWrite(Motor_R_dir_pin,Motor_forward);  
  digitalWrite(Motor_L_dir_pin,Motor_backward);
  delay(2000);
  digitalWrite(Motor_R_dir_pin,Motor_backward);  
  digitalWrite(Motor_L_dir_pin,Motor_forward);
  delay(2000);
}


void motor_test(){
  int pwm_R=0; 
  int pwm_L=0;   
  for(int i = 99;i<100;i--){  
    //Direction 
    digitalWrite(Motor_R_dir_pin,Motor_backward);   
    digitalWrite(Motor_L_dir_pin,Motor_backward); 
    delay(2000); 
    digitalWrite(Motor_R_dir_pin,Motor_forward);   
    digitalWrite(Motor_L_dir_pin,Motor_forward);  
    delay(2000); 
    //Speed
    int pwm_R=0; 
    int pwm_L=0;  
    pwm_L = i; 
    pwm_R = i; 
    analogWrite(Motor_L_pwm_pin,pwm_L); 
    analogWrite(Motor_R_pwm_pin,pwm_R);             
  }
}

void joystickPressed() { 
  button_pressed = !button_pressed; 
  button_count++; 
} 

 
