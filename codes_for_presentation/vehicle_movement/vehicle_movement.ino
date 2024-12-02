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
const int debounce_time = 50;
volatile boolean joystick_mode = true;
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
const float wheel_dia_m = 0.037;
const float wheel_circ = PI * wheel_dia_m;
const int pulse_per_revolution = 210;
const float pulse_per_cm = (float)pulse_per_revolution / (wheel_circ * 100);
volatile int pulse_count_R = 0, pulse_count_L = 0;
volatile float dist_L = 0, dist_R = 0; // distance travelled by each motor
float distance_travelled = 0; // average distance travelled of both motors
  
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
const uint8_t CMPS14_address = 0x60;
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
  update_lcd();
  current_degree = compass_logic();
  if (joystick_mode) {
    joystick_movement();
  } else {
    get_commands();
  }
}

// Refreshes LCD with non-disruptive delay
void update_lcd(){
  unsigned long current_millis_lcd = millis();
  if (current_millis_lcd - prev_millis_lcd >= lcd_interval) {
    prev_millis_lcd = current_millis_lcd;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Mode: ");
    lcd.print(joystick_mode ? "Joystick" : "ESP");
    
    lcd.setCursor(0,1);
    lcd.print("Dir: ");
    lcd.print(get_direction(current_degree));
    lcd.print(", Deg: ");
    lcd.print(current_degree);
    
    lcd.setCursor(0,2);
    lcd.print("P_L:");
    lcd.print(pulse_count_L);
    lcd.print(",D_L:");
    lcd.print(dist_L);
    lcd.print(",P_R:");
    lcd.print(pulse_count_R);
    lcd.print(",D_R:");
    lcd.print(dist_R);
    
    lcd.setCursor(0,3);
    if (joystick_mode) {
      lcd.print("X: ");
      lcd.print(joy_x_value);
      lcd.print(", Y: ");
      lcd.print(joy_y_value);
    } else {
      lcd.print("Cmd: ");
      lcd.print(message);
    }
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

    // Only triggered on startup
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
// Available commands: dist, drive, move; deg, turn
void get_commands() {
  if (Serial.available() > 0){
    message = Serial.readStringUntil('\n'); 
    int delim = message.indexOf(":");
    
    if (delim > -1) {
      String command = message.substring(0, delim);
      command.toLowerCase();

      int value = message.substring(delim + 1).toInt();
      
      if (command.startsWith("dist") 
      || command.startsWith("drive") 
      || command.startsWith("move")
      && value != 0){
          travel_distance(value, motor_max_speed);
      } else if (command.startsWith("deg") || command.startsWith("turn")) {
          rotate_vehicle(value);
      } else {
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
  // Reset all related counters
  {
    pulse_count_R = pulse_count_L = dist_R = dist_L = 0;
  }
  
  // Flip motor direction depending on value of distance
  byte motor_direction = distance > 0 ? 0 : 1;

  while (distance_travelled < abs(distance)){
    distance_travelled = (dist_R + dist_L) / 2;
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

//There must be a easier way...
// Controls motor movement by joystick inputs
void joystick_movement() {
    joy_x_value = analogRead(joy_x);
    joy_y_value = analogRead(joy_y);
    
    // Speed Control
    motor_speed_x = (joy_x_value < stationary_x)
                  ? map(joy_x_value, 0, stationary_x, 255, 0)     // Forward (X-axis)
                  : map(joy_x_value, stationary_x, 1023, 0, 255); // Backward (X-axis)
                  
    motor_speed_y = (joy_y_value < stationary_y)
                  ? map(joy_y_value, 0, stationary_y, 255, 0)     // Right (Y-axis)
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
  dist_R = pulse_count_R / pulse_per_cm;
}
// Left motor's encoder pulse counter
void encoder_pulse_L(){
  pulse_count_L++;
  dist_L = pulse_count_L / pulse_per_cm;
}

// Trigger function on joystick press
void joystick_interrupt() {
  unsigned long current_time = millis();
  if (current_time - last_button_press_time > debounce_time) {
    joystick_mode = !joystick_mode;
    last_button_press_time = current_time;
  }
}
