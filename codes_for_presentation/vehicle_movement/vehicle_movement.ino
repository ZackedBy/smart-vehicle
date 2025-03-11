/*
 * #define used ONLY for pin declarations
 */

#include <LiquidCrystal.h>
#include <Wire.h>
#include <LIDARLite.h>
#include "Adafruit_TCS34725.h"

// JoyStick Variables
#define joy_x A2
#define joy_y A1
#define joy_key 18
int joy_x_value = 0, joy_y_value = 0;

// JoyStick Press Variables
const int debounce_time = 50;
volatile boolean joystick_mode = true;
volatile unsigned long last_button_press_time = 0;

// Motor Variables
#define Motor_L_dir_pin 7
#define Motor_R_dir_pin 8
#define Motor_L_pwm_pin 9
#define Motor_R_pwm_pin 10
const byte Motor_forward = 0, Motor_backward = 1;
const int stationary_x = 502, stationary_y = 506, motor_max_speed = 255;
int motor_speed_x = 0, motor_speed_y = 0;

// Encoder Variables
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

// LCD Variables
#define rs 32
#define en 30
#define d4 28
#define d5 26
#define d6 24
#define d7 22
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned long prev_millis_lcd = 0;

// Compass Variables
const uint8_t CMPS14_address = 0x60;
int current_degree = 0, target_degreeree = 0, last_used_degree = 0, prev_checked_degree = 0;
int direction_difference, compass_offset;
unsigned long prev_error_check = 0;

// LiDAR Variables
#define lidar_pot A3
LIDARLite lidar;
int lidar_dist = 0;       // Average distance after x amount of readings
const int num_readings = 20;  // X amount of readings
int lidar_dist_threshold = 0;

//RGB Sensor Variables
//#define i2c_address (0x29)
Adafruit_TCS34725 rgb_sens = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
float r, g, b;
String color = "black";

// Website Variables
String message; //whole message sent from website

void setup() {
  attachInterrupt(digitalPinToInterrupt(joy_key), joystick_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_R), encoder_pulse_R, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_L), encoder_pulse_L, RISING);

  // includes Wire.begin
  if (rgb_sens.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }

  lcd.begin(20,4);

  Serial.begin(9600);  // For uploading code/monitoring "prints"
  Serial2.begin(9600); // For sending/receiving website data
  //Serial2.setTimeout(500); // Reduce timeout so reading doesn't block

  lidar.begin(0, true);
  lidar.configure(0); //change to 1 for short range, high speed

  compass_offset = compass_logic(); // Get compass reading on start-up
}

void loop(){
  get_rgb_value();
  display_lcd();
  current_degree = compass_logic();
  lidar_dist = get_lidar_distance(num_readings);
  //lidar_dist_threshold = map(analogRead(lidar_pot), 0, 1023, 5, 40);

  if (joystick_mode) {
    last_used_degree = current_degree;
    joystick_movement();
  } else {
    rotate_vehicle(last_used_degree); // Self-correct rotation
    get_commands_from_web();
  }
  send_info_to_web();
}
//==================================================================LiDAR Functions=============================================

// TO-DO: Change for loop to boolean and counter
int get_lidar_distance(int readings) {
  int lidar_dist_sum = 0;
  for (int i = 0; i < readings; i++) {
    if (i == 0) {
      lidar_dist_sum += lidar.distance(true);
    } else {
      lidar_dist_sum += lidar.distance(false);
    }
  }
  return (int)(lidar_dist_sum / readings);
}

// EXERCISE 3
void drive_prox_check() {
  //int lidar_dist_threshold = num_readings;
  //int lidar_dist_threshold = map(analogRead(lidar_pot), 0, 1023, 5, 40);
  while (true) {
    lidar_dist = get_lidar_distance(num_readings);
    byte motor_direction = lidar_dist > lidar_dist_threshold ? Motor_forward : Motor_backward;
    display_lcd();
    move_motor(motor_direction, motor_direction, motor_max_speed, motor_max_speed);

    if (joystick_mode) break; //needs dedicated button for loop extraction
  }
}

// EXERCISE 5
void move_to_threshold(int deg, int threshold) {
  rotate_vehicle(deg);
  while (get_lidar_distance(num_readings) > threshold) {
    move_motor(Motor_forward, Motor_forward, motor_max_speed, motor_max_speed);
    current_degree = compass_logic();
  }
  move_motor(0, 0, 0, 0);
}

void follow_path() {
  move_to_threshold(0, 40);
  move_to_threshold(270, 125);
  move_to_threshold(180, 113);
  move_to_threshold(90, 165);
  joystick_mode = true;
}
//========================================================================RGB Functions================================================

void get_rgb_value(){
  //uint16_t clear, red, green, blue;
  //rgb_sens.setInterrupt(true);
  rgb_sens.getRGB(&r, &g, &b);
  //rgb_sens.setInterrupt(false);

//  Serial.print("\tR:\t"); Serial.print(r);
//  Serial.print("\tG:\t"); Serial.print(g);
//  Serial.print("\tB:\t"); Serial.print(b);
//  Serial.println("\t");
//
//  Serial.print("\t");
//  Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);
//  Serial.println();
  color = "black";
  if(r > 100 && r > g  && r > b) {
    color = "red";
  } else if (b > g  && b > r) {
    color = g > 80 ? "green" : "blue";
  }
}

//==================================================================LCD Specific Functions=============================================

// Displays and refreshes LCD with non-disruptive delay
void display_lcd(){
  if (millis() - prev_millis_lcd >= 500) {
    prev_millis_lcd = millis();
    lcd.clear();
    
    lcd.setCursor(0,0);
    lcd.print("Mode: "); lcd.print(joystick_mode ? "Joystick" : "ESP");

    lcd.setCursor(0,1);
    lcd.print(get_direction(current_degree)); lcd.print(": "); lcd.print(current_degree);
    lcd.print(" | Col: "); lcd.print(color);

    lcd.setCursor(0,2);
    lcd.print("D: "); lcd.print(lidar_dist);
    lcd.print(" | "); 
    lcd.print(int(r)); lcd.print(" "); 
    lcd.print(int(g)); lcd.print(" "); 
    lcd.print(int(b));

    lcd.setCursor(0,3);
    if (joystick_mode) {
      lcd.print("X: "); lcd.print(joy_x_value);
      lcd.print(", Y: "); lcd.print(joy_y_value);
    } else {
      lcd.print(message);
    }
  }
}

// Converts current direction to cardinal point
String get_direction(int heading) {
    const String directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return directions[((heading + 23) % 360) / 45]; // Map to nearest direction
}

//========================================================================Website Functions========================================================

// Controls vehicle with serial inputs
// Available commands: dist, drive, move; deg, turn
void get_commands_from_web() {
  if(Serial2.available() > 0) {

    message = Serial2.readStringUntil('\n');
    message.trim();
    Serial.println(message);

    int delim = message.indexOf(":");

    if (delim > -1) {
      String command = message.substring(0, delim);
      command.toLowerCase();
      int value = message.substring(delim + 1).toInt();

      if ((command.startsWith("dist")
      || command.startsWith("drive")
      || command.startsWith("move"))
      && value != 0) {
          travel_distance(value, motor_max_speed);
      } else if (command.startsWith("deg") || command.startsWith("turn")) {
          rotate_vehicle(value);
      } else {
        Serial.println("Command not found: " + message);
      }
    }
  }
}

void send_info_to_web(){
  Serial2.println("LIDAR:" + String(lidar_dist));
  Serial2.println("COMP:" + String(current_degree));
  Serial2.println("RGB:" + color);
}
//========================================================================Compass Functions========================================================

// Sets starting direction as 0 degrees and updates degree value if turning
int compass_logic() {
  Wire.beginTransmission(CMPS14_address);
  Wire.write(1);
  Wire.endTransmission(false);
  Wire.requestFrom(CMPS14_address, 1, true);

  if (Wire.available() >= 1){
    byte raw = Wire.read();
    int mapped_heading = map(raw, 0, 255, 0, 360);

    // Offset compass reading with initial reading on startup
    int corrected_heading = mapped_heading - compass_offset;

    // Normalise the reading [0, 360]
    if (corrected_heading < 0) {
      corrected_heading += 360;
    } else if (corrected_heading >= 360) {
      corrected_heading -= 360;
    }

    return corrected_heading;
  }
  return 0;
}

//==================================================================Motor Movement Functions========================================

// Move each motor in certain direction and speed
void move_motor(byte left, byte right, int l_speed, int r_speed){
  //Direction
  digitalWrite(Motor_L_dir_pin, left);
  digitalWrite(Motor_R_dir_pin, right);
  //Speed
  analogWrite(Motor_L_pwm_pin, l_speed);
  analogWrite(Motor_R_pwm_pin, r_speed);
}

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
        move_motor(Motor_backward, Motor_forward, motor_speed_y, motor_speed_y);
    } else if (x_in_deadzone && joy_y_value < stationary_y) {                   // Move right
        move_motor(Motor_forward, Motor_backward, motor_speed_y, motor_speed_y);
    } else if (y_in_deadzone && joy_x_value < stationary_x) {                   // Move forward
        move_motor(Motor_forward, Motor_forward, motor_speed_x, motor_speed_x);
    } else if (y_in_deadzone && joy_x_value > stationary_x) {                   // Move backward
        move_motor(Motor_backward, Motor_backward, motor_speed_x, motor_speed_x);
    } else if (joy_x_value > stationary_x && joy_y_value > stationary_y) {      // Backward-left
        move_motor(Motor_backward, Motor_backward, 0, motor_speed_x);
    } else if (joy_x_value > stationary_x && joy_y_value < stationary_y) {      // Backward-right
        move_motor(Motor_backward, Motor_backward, motor_speed_x, 0);
    } else if (joy_x_value < stationary_x && joy_y_value > stationary_y) {      // Forward-left
        move_motor(Motor_forward, Motor_forward, 0, motor_speed_x);
    } else if (joy_x_value < stationary_x && joy_y_value < stationary_y) {      // Forward-right
        move_motor(Motor_forward, Motor_forward, motor_speed_x, 0);
    } else {                                                                    // Stationary (dead zone)
        move_motor(0, 0, 0, 0);
    }
}

// Normalises difference to [-180, 180]
int get_diff(int target) {
  int diff = target - current_degree;
  if (diff > 180) {
    diff -= 360;
  } else if (diff < -180) {
    diff += 360;
  }
  return diff;
}

// Rotates the vehicle with shortest path
void rotate_vehicle(int target_degree) {
  int proximity_tolerance = 2;        // Accepts +/- 2 from target_degree
  int start_degree = current_degree;  // Stores degree when command is given
  last_used_degree = target_degree;   // Update auto-rotate degree
  prev_error_check = 0;
  bool tried_reverse = false;

  // Initial distance to rotate
  int direction_difference = get_diff(target_degree);

  // Set the initial rotation direction based on the shortest path
  bool rotating_clockwise = direction_difference > 0;

  // Loops until difference between target and current is less than tolerance
  while (abs(direction_difference) >= proximity_tolerance) {
    
    display_lcd();
    current_degree = compass_logic();
    direction_difference = get_diff(target_degree);

    // Check if turning is blocked
    if (is_turning_blocked()) {
      if (tried_reverse) {
        last_used_degree = start_degree; // Update auto-rotate back to same degree as when function was called
        break;
      }
      prev_error_check = millis();
      message = "Block. Trying rev...";
      tried_reverse = true;
      rotating_clockwise = !rotating_clockwise; // Flip direction after blockage
    }

    // Reduce rotation speed for last few degrees
    int rotation_speed = abs(direction_difference) > 10
      ? motor_max_speed
      : (int)motor_max_speed / 4;

    // Rotate vehicle in the new direction
    if (rotating_clockwise) {
      move_motor(Motor_forward, Motor_backward, rotation_speed, rotation_speed); // Move CW
    } else {
      move_motor(Motor_backward, Motor_forward, rotation_speed, rotation_speed); // Move CCW
    }
  }

  move_motor(0, 0, 0, 0); // Stop movement
  message = "Turn:" + String(target_degree); // Reset message
}


bool is_turning_blocked() {
  const unsigned long error_timeout = 500; // 500ms timeout
  const int degree_threshold = 10;

  // Store initial degree to compare later
  if (prev_error_check == 0) {
    prev_error_check = millis();
    prev_checked_degree = current_degree;
  }
  
  // Checks progress every timeout
  if (millis() - prev_error_check >= error_timeout) {
  
    if (abs(current_degree - prev_checked_degree) <= degree_threshold ) {
      // No progress detected -> blockage
      return true;
    } else {
      // Progress detected, reset the monitor
      prev_checked_degree = current_degree;
      prev_error_check = millis();
    }
    
  }
  return false;
}

// Drives the vehicle from an input distance and speed
void travel_distance(int distance, int motor_speed){
  reset_dist_variables();
  byte motor_direction = distance > 0 ? Motor_forward : Motor_backward;
  bool tried_all_paths = false;

  while (distance_travelled <= abs(distance)) {
    lidar_dist = get_lidar_distance(num_readings);

    // Checks if path is blocked
    if (lidar_dist < 10 && distance > 0 && !tried_all_paths) {
      motor_direction = check_path_and_turn();
      if (motor_direction == Motor_backward) {
        tried_all_paths = true;
      }
    }

    // Adjust speed based on lidar readings
    int var_speed = (lidar_dist >= 10 && lidar_dist < 30) ? (0.6 * motor_speed) : motor_speed;
    // Adjust speed based on color under rgb sensor
    //var_speed = change_speed_for_color(var_speed);
    // Change to reverse -5?
    //if (var_speed == 0) break; // Stop car if red underneath
    
    distance_travelled = (dist_R + dist_L) / 2;

    display_lcd();
    move_motor(motor_direction, motor_direction, var_speed, var_speed);
  }
  move_motor(0, 0, 0, 0);
}

int change_speed_for_color(int m_speed) {
  get_rgb_value();
  if (color == "red"){
    return 0;
  } else if (color == "green") {
    return m_speed;
  } else if (color == "blue") {
    return (int) m_speed / 2;
  }
  return m_speed;
}

byte check_path_and_turn(){   
  move_motor(0, 0, 0, 0);
  // Detach interrupts so encoders don't increase distance_travelled
  detachInterrupt(digitalPinToInterrupt(ENC_A_R));
  detachInterrupt(digitalPinToInterrupt(ENC_A_L));
  byte new_direction = Motor_forward;

  for (int i = 0; i < 3; i++) {
    if (i == 0 || i == 2) {  // First try turning right OR turn right again if left fails
      rotate_vehicle(current_degree + 90);
      // Drive in reverse if all paths are blocked
      if (i == 2) {
        new_direction = Motor_backward;
        break;
      }
    } else {  // If right fails, try turning left
        rotate_vehicle(current_degree - 90);
        rotate_vehicle(current_degree - 90);
    }

    // If a path is clear after turning, restore initial distance values
    if (get_lidar_distance(num_readings) >= 10) {
      break;
    }
  }
  // Reattach interrupts for distance_travelled calculation
  attachInterrupt(digitalPinToInterrupt(ENC_A_R), encoder_pulse_R, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_L), encoder_pulse_L, RISING);

  return new_direction;
}


//==========================================================Interrupt Functions=============================================

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
    message = "";
    reset_dist_variables();
    joystick_mode = !joystick_mode;
    last_button_press_time = current_time;
  }
}

//===========================================================Misc./Utility Functions========================================

// Helper function to reset variables related to distance calculation
void reset_dist_variables() {
  pulse_count_R = 0;
  pulse_count_L = 0;
  dist_R = 0;
  dist_L = 0;
  distance_travelled = 0;
}
