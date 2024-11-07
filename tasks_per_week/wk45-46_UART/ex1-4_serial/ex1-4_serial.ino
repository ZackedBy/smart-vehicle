/*
 * #define used ONLY for pin declarations
 */

#include <LiquidCrystal.h>
#include <Wire.h>

//JoyStick Variables
#define joy_x A2
#define joy_y A1
#define joy_key 18
int joy_x_value = 0, joy_y_value = 0;
double x_volt = 0, y_volt = 0;

//JoyStick Press Variables
#define led_pin 13
volatile int button_count = 0;
const int debounce_time = 50;
volatile boolean button_pressed = false;
volatile unsigned long last_button_press_time = 0;

//Motor Variables
#define Motor_L_dir_pin 7
#define Motor_R_dir_pin 8
#define Motor_L_pwm_pin 9
#define Motor_R_pwm_pin 10
const byte Motor_forward = 0, Motor_backward = 1;
const int stationary_x = 511, stationary_y = 519, motor_max_speed = 255; 
int motor_speed_x = 0, motor_speed_y = 0;

//Encoder Variables
#define ENC_A_R 2  
#define ENC_A_L 3
#define ENC_B_R 11
#define ENC_B_L 12
volatile int pulse_count_R = 0, pulse_count_L = 0;
const float wheel_dia_m = 0.037; //Find actual value
const float wheel_circ = PI * wheel_dia_m;
const int pulse_per_revolution = 210; //Find actual value
const float pulse_per_cm = (float)pulse_per_revolution / (wheel_circ * 100);
float distance_travelled = 0;
//const float target_distance = 30.0;


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

//Compass Variables
//const uint8_t CMPS14_address = 0x60; //can use #define too
#define CMPS14_address 0x60 //can use #define too
byte raw;
int current_degree = 0, target_degree = 0;
int direction_difference;
const int degrees_tolerance = 3;
int compass_offset;


//Serial Reading
String message;

void setup() {
  Wire.begin();
  lcd.begin(20,4);
  attachInterrupt(digitalPinToInterrupt(joy_key), joystick_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_R), encoder_pulse_R, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_L), encoder_pulse_L, RISING);
  Serial.begin(9600);
  compass_offset = compass_logic();
}

void loop(){  
  current_degree = compass_logic();
  update_lcd();
  //rotate_vehicle(0);
  get_commands();
  if (button_pressed){
    button_pressed = false;
    Serial.println("boop");
  }
}

// Sets starting direction as 0 degrees and updates degree value if turning
int compass_logic() {
  Wire.beginTransmission(CMPS14_address);
  Wire.write(1);
  Wire.endTransmission(false);
  Wire.requestFrom(CMPS14_address, 1, true);
  
  if (Wire.available() >= 1){
    raw = Wire.read();
    int mapped_heading = map(raw, 0, 255, 0, 360);
    
    int corrected_heading = mapped_heading - compass_offset;

    if (corrected_heading < 0) {
      corrected_heading += 360;
    } else if (corrected_heading >= 360) {
      corrected_heading -= 360;
    }

    return corrected_heading;
  }
  return 0;
}

// Controls vehicle with serial inputs
void get_commands() {
  if (Serial.available() > 0){
    message = Serial.readStringUntil('\n'); 
    int delim = message.indexOf(":");
    
    if (delim > -1) {
      int value = message.substring(delim + 1);
      String command = message.substring(0, delim).toLowerCase();
      if (command.startsWith("dist") 
      || command.startsWith("drive") 
      && value.toInt() != 0){
          travel_distance(value.toInt(), motor_max_speed);
      } else if (command.startsWith("deg")) {
          rotate_vehicle(value.toInt());
      } else if (command.startsWith("turn")) {
        String dir = value.toLowerCase();
        if (dir.startsWith("n")) {
          rotate_vehicle(0);
        } else if (dir.startsWith("e")) {
          rotate_vehicle(90);
        } else if (dir.startsWith("s")) {
          rotate_vehicle(180);
        } else if (dir.startsWith("w")) {
          rotate_vehicle(90);
        } else {
          Serial.println("Invalid direction. Use N, E, S, or W.");
        }
      }
      
      else {
        Serial.println("Command not found: " + message);
      }
    } else {
      Serial.println("Incorrect command. Try \"dist:\" or \"degree:\".");
    }
  }
}

// Drives the vehicle from an input distance and speed
void travel_distance(int distance, int max_speed){
  distance_travelled = 0;
  pulse_count_R = 0;
  pulse_count_L = 0;
 
  byte motor_direction = distance > 0 ? 0 : 1;

  while (distance_travelled < distance){
    distance_travelled = ( (pulse_count_L + pulse_count_R) / 2.0) / pulse_per_cm;
    update_lcd();
    move_motor(motor_direction, motor_direction, max_speed, max_speed);
  }
  move_motor(0, 0, 0, 0);
}

// Rotates the vehicle with shortest path
void rotate_vehicle(int target_deg) {
  while (true) {
    update_lcd();
    current_degree = compass_logic();
    
    direction_difference = target_deg - current_degree;
    
    if (direction_difference > 180) {
      direction_difference -= 360;
    } else if (direction_difference < -180) {
      direction_difference += 360;
    }

    if (abs(direction_difference) <= degrees_tolerance) {
      break;
    }

    if (direction_difference > 0) {
      // Move CW
      move_motor(Motor_forward, Motor_backward, motor_max_speed, motor_max_speed);
    } else {
      // Move CCW
      move_motor(Motor_backward, Motor_forward, motor_max_speed, motor_max_speed);
    }
  }
  move_motor(0, 0, 0, 0);
}

// Converts current direction to cardinal point
String get_direction(int heading) {
  if ((heading >= 338 && heading <= 360) || (heading >= 0 && heading < 23)) {
    return "N";
  } else if (heading >= 23 && heading < 68) {
    return "NE";
  } else if (heading >= 68 && heading < 113) {
    return "E";
  } else if (heading >= 113 && heading < 158) {
    return "SE";
  } else if (heading >= 158 && heading < 203) {
    return "S";
  } else if (heading >= 203 && heading < 248) {
    return "SW";
  } else if (heading >= 248 && heading < 293) {
    return "W";
  } else if (heading >= 293 && heading < 338) {
    return "NW";
  } else {
    return "Invalid";
  }
}

// Refreshes LCD with non-disruptive delay
void update_lcd(){
  unsigned long current_millis_lcd = millis();
  if (current_millis_lcd - prev_millis_lcd >= lcd_interval) {
    prev_millis_lcd = current_millis_lcd;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Command: " + message);
    lcd.setCursor(0,1);
    lcd.print("Direction: " + get_direction(current_degree));
    lcd.setCursor(0,2);
    lcd.print("Cur: " + String(current_degree) + ", Tar: " + String(target_degree));
    lcd.setCursor(0,3);
    lcd.print("Travel " + String(distance_travelled));
  }
}

// Move each motor in certain direction and speed
void move_motor(byte left, byte right, int l_speed, int r_speed){
  //Direction
  digitalWrite(Motor_L_dir_pin, left);
  digitalWrite(Motor_R_dir_pin, right);
  //Speed
  analogWrite(Motor_L_pwm_pin, l_speed); 
  analogWrite(Motor_R_pwm_pin, r_speed); 
}

// Right motor's encoder pulse counter
void encoder_pulse_R(){
  pulse_count_R++;
}
// Left motor's encoder pulse counter
void encoder_pulse_L(){
  pulse_count_L++;
}

// Trigger function on joystick press
void joystick_interrupt() {
  unsigned long current_time = millis();
  if (current_time - last_button_press_time > debounce_time) {
    button_count++;
    button_pressed = true;
    last_button_press_time = current_time;
  }
}
