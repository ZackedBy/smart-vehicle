/*
 * #define used ONLY for pin declarations
 */

#include <LiquidCrystal.h>
#include <Wire.h>
#include <LIDARLite.h>
#include "Adafruit_TCS34725.h"
#include <EEPROM.h>

// JoyStick Press Variables
#define joy_key 18
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
volatile int pulse_count_R = 0, pulse_count_L = 0;
float distance_travelled = 0; // average distance travelled of both motors
const byte ENC_ADDR = 0x00;
float cm_per_enc_pulse = 0.0;
float saved_enc_value;

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
LIDARLite lidar;
int lidar_dist = 0;       // Average distance after x amount of readings
const int num_readings = 20;  // X amount of readings

//RGB Sensor Variables
//#define i2c_address (0x29)
Adafruit_TCS34725 rgb_sens = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X);
float r, g, b;
String color = "black";

// Website Variables
String message; //whole message sent from website


struct Position {
  int x;
  int y;
};

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


//========================================================================RGB Functions================================================
struct Color {
  int r,g,b;
  String toString() {
    return "R: " + String(r) + ", G: " + String(g) + ", B: " + String(b);
  }
};

Color calibratedColors[3];
volatile int presses = 0;
volatile boolean calibrated = true;

void cal_col() {
  rgb_sens.getRGB(&r, &g, &b);
  if (presses >= 1 && presses <= 3) {
    calibratedColors[presses - 1] = {int(r), int(g), int(b)};
    Serial.print("Calibrated Color ");
    Serial.print(presses);
    Serial.print(": ");
    Serial.println(calibratedColors[presses - 1].toString());
    calibrated = true;
  }
}

bool isColorMatch(Color target, int threshold = 30) {
  return (abs(int(r) - target.r) <= threshold) &&
         (abs(int(g) - target.g) <= threshold) &&
         (abs(int(b) - target.b) <= threshold);
}

void get_rgb_value() {
  rgb_sens.getRGB(&r, &g, &b);
  color = "black";
  if (isColorMatch(calibratedColors[0])) {
    color = "red";
  } else if (isColorMatch(calibratedColors[1])) {
    color = "green";
  } else if (isColorMatch(calibratedColors[2])) {
    color = "blue";
  } else {
    color = "black";
  }
}

int color_speed() {
  if (color == "blue") {
    return (int) motor_max_speed * 0.35;
  } else if (color == "green") {
    return (int) motor_max_speed * 0.75;
  } 
  return 0;
}

//==================================================================LCD Specific Functions=============================================

// Displays and refreshes LCD with non-disruptive delay
void display_lcd(){
  if (millis() - prev_millis_lcd >= 500) {
    prev_millis_lcd = millis();
    lcd.clear();
    
    lcd.setCursor(0,0);
    lcd.print("Mode: "); lcd.print(joystick_mode ? "Joystick" : "ESP");

   // lcd.setCursor(0,1);
//    lcd.print(get_direction(current_degree)); lcd.print(": "); lcd.print(current_degree);
//    lcd.print(" | Col: "); lcd.print(color);

    lcd.setCursor(0,1);
    lcd.print("D: "); lcd.print(lidar_dist);
    lcd.print(" | "); 
    lcd.print(int(r)); lcd.print(" "); 
    lcd.print(int(g)); lcd.print(" "); 
    lcd.print(int(b));

    lcd.setCursor(0,2);
    lcd.print("R: ");
    lcd.print(pulse_count_R);
    lcd.print(", L: ");
    lcd.print(pulse_count_L);
    
    lcd.setCursor(0,3);
    lcd.print("P/cm: ");
    lcd.print(saved_enc_value, 4);
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
    message.toLowerCase();
    Serial.println(message);

    if (message.startsWith("go")) {
      doMaze();
    } else if (message.startsWith("calibrate")) {
      cal_enc(); // DO THIS YES
    } else if (message.startsWith("move")) {
      int delim = message.indexOf(":");
      int value = message.substring(delim + 1).toInt();
      travel_distance(value, 255);
    } else {
      Serial.println("Command not found: " + message);
    }
  }
}

void send_info_to_web(){
  Serial2.println("LIDAR:" + String(lidar_dist));
  Serial2.println("COMP:" + String(current_degree));
  Serial2.println("RGB:" + color);

 
  Serial2.print("ENC:");
  Serial2.println(saved_enc_value);
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
  prev_error_check = 0;
  
  // Initial distance to rotate
  int direction_difference = get_diff(target_degree);

  // Set the initial rotation direction based on the shortest path
  bool rotating_clockwise = direction_difference > 0;

  // Detach interrupts so encoders don't increase distance_travelled
  detachInterrupt(digitalPinToInterrupt(ENC_A_R));
  detachInterrupt(digitalPinToInterrupt(ENC_A_L));

  // Loops until difference between target and current is less than tolerance
  while (abs(direction_difference) >= proximity_tolerance) {
    current_degree = compass_logic();               // update compass reading
    direction_difference = get_diff(target_degree);
    if (abs(direction_difference) <= proximity_tolerance) {
      break; //break early if good
    }
    
    // Reduce rotation speed for last few degrees
    int rotation_speed = abs(direction_difference) > 10
      ? motor_max_speed
      : (int)motor_max_speed / 4;
    
    // lidar_dist = get_lidar_distance(1);  // update lidar reading
    send_info_to_web();
    display_lcd();

    // Check if turning is blocked
    if (is_turning_blocked()) {
      //drives without checking lidar
      drive(-10, Motor_backward);
      //updates error so as not to trigger is_turning_blocked
      prev_error_check = millis();
    }

    // Rotate vehicle in the new direction
    if (rotating_clockwise) {
      move_motor(Motor_forward, Motor_backward, rotation_speed, rotation_speed); // Move CW
    } else {
      move_motor(Motor_backward, Motor_forward, rotation_speed, rotation_speed); // Move CCW
    }
  }
  
  // Reattach interrupts for distance_travelled calculation
  attachInterrupt(digitalPinToInterrupt(ENC_A_R), encoder_pulse_R, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_L), encoder_pulse_L, RISING);
  
  move_motor(0, 0, 0, 0); // Stop movement
  message = "Turn:" + String(target_degree); // Reset message
}


bool is_turning_blocked() {
  const unsigned long error_timeout = 300; // 500ms timeout
  const int degree_threshold = 2;

  // Store initial degree to compare later
  if (prev_error_check == 0) {
    prev_error_check = millis();
    prev_checked_degree = current_degree;
  }
  
  // Checks progress every timeout
  if (millis() - prev_error_check >= error_timeout) {
    prev_error_check = millis();
    if (abs(current_degree - prev_checked_degree) <= degree_threshold ) {
      // No progress detected -> blockage
      return true;
    } else {
      prev_checked_degree = current_degree;
    }
  }
  return false;
}

// Drives the vehicle from an input distance and speed
void travel_distance(int distance, int motor_speed){
  reset_dist_variables();
  byte motor_direction = distance > 0 ? Motor_forward : Motor_backward;

  while (distance_travelled <= abs(distance)) {
    
    distance_travelled = ((pulse_count_R + pulse_count_L) / 2) * saved_enc_value;
    Serial.println(distance_travelled);
    
    // Checks if path is blocked
    if (lidar_dist < 10 && distance > 0) {
      check_path_and_turn();
    }

    // Adjust speed based on lidar readings
    int var_speed = (lidar_dist >= 10 && lidar_dist < 30) ? (0.6 * motor_speed) : motor_speed;
    
    // Breaks out if color detected
    if (color != "black") {
      break;
    } 

    get_rgb_value();
    lidar_dist = get_lidar_distance(num_readings);
    send_info_to_web();
    display_lcd();
    move_motor(motor_direction, motor_direction, var_speed, var_speed);
  }
  move_motor(0, 0, 0, 0);
}

void check_path_and_turn(){   
  move_motor(0, 0, 0, 0);
  // Detach interrupts so encoders don't increase distance_travelled
  detachInterrupt(digitalPinToInterrupt(ENC_A_R));
  detachInterrupt(digitalPinToInterrupt(ENC_A_L));

  //i = 0, i <= 360; i + 90
  for (int i = 0; i < 3; i++) {
    rotate_vehicle(current_degree + 90);
    // If a path is clear after turning, restore initial distance values
    if (get_lidar_distance(num_readings) >= 10) {
      break;
    }
  }
  
  // Reattach interrupts for distance_travelled calculation
  attachInterrupt(digitalPinToInterrupt(ENC_A_R), encoder_pulse_R, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_L), encoder_pulse_L, RISING);
}


//==========================================================Interrupt Functions=============================================

// Right motor's encoder pulse counter
void encoder_pulse_R(){
  pulse_count_R++;
  //dist_R = pulse_count_R / pulse_per_cm;
}
// Left motor's encoder pulse counter
void encoder_pulse_L(){
  pulse_count_L++;
  //dist_L = pulse_count_L / pulse_per_cm;
}

// Trigger function on joystick press
void joystick_interrupt() {
  unsigned long current_time = millis();
  if (current_time - last_button_press_time > debounce_time) {
    message = "";
    reset_dist_variables();
    presses++;
    calibrated = false;
    last_button_press_time = current_time;
  }
}

//===========================================================Misc./Utility Functions========================================

// Helper function to reset variables related to distance calculation
void reset_dist_variables() {
  pulse_count_R = 0;
  pulse_count_L = 0;
  distance_travelled = 0;
}

// Drive without error checking
void drive(int distance, byte dir) {
  reset_dist_variables();
  while (distance_travelled <= abs(distance)) {
    distance_travelled = ((pulse_count_R + pulse_count_L) / 2) * saved_enc_value;
    display_lcd();
    move_motor(dir, dir, motor_max_speed, motor_max_speed);
  }
  move_motor(0, 0, 0, 0);
}

void cal_enc() {
  rotate_vehicle(0);
  reset_dist_variables();
  
  cm_per_enc_pulse = 0.0;
  volatile int pulse_avg = 0;
  int start_lidar_dist = get_lidar_distance(20);
  int curr_lidar_dist = 0;
  const int distance_to_travel = 20;
  
  move_motor(Motor_forward, Motor_forward, 122, 122);
  while(true) {
    curr_lidar_dist = get_lidar_distance(20);
    pulse_avg = (pulse_count_L + pulse_count_R) / 2;
    if (abs(start_lidar_dist - curr_lidar_dist) >= distance_to_travel) {
      //save to EEPROM
      cm_per_enc_pulse = (float) distance_to_travel / pulse_avg;
      EEPROM.put(ENC_ADDR, cm_per_enc_pulse);
      break;
    }
  } 
  EEPROM.get(ENC_ADDR, saved_enc_value);
  move_motor(0, 0, 0, 0);
}


Position getStartingPosition() {
  Position pos = {0, 0};
  
  rotate_vehicle(0); // turn to North
  pos.y = get_lidar_distance(20); // Set y position from lidar
  rotate_vehicle(90); // Turn to East
  pos.x = get_lidar_distance(20);// Set x position from lidar
  
  rotate_vehicle(270); // Turn to West to begin maze traversal
  
  return pos;
}

// Make top right origin (0,0)
void doMaze() {
  const int maze_length = 202; // Internal length (↔) of maze
  const int maze_width = 120; // Internal width (↕) of maze

  Position final_pos = {100, 180}; // Update when known
  Position current_pos = getStartingPosition();

  int start_speed = (int) motor_max_speed * 0.5;
  int blue_speed = (int) motor_max_speed * 0.35;
  int green_speed = (int) motor_max_speed * 0.75;
  int var_speed = start_speed;

  const int threshold = 2; // allow for final position to be off by 2 cm

  while(true) {
    int dx = final_pos.x - current_pos.x; // Get x difference
    int dy = final_pos.y - current_pos.y; // Get y difference

    // Goal has been reache: Stop
    if (abs(dx) <= threshold && abs(dy) <= threshold) {
      break;
    }
    
    get_rgb_value(); // Get color from RGB sensor
    lidar_dist = get_lidar_distance(num_readings); // Get distance value from lidar
    current_degree = compass_logic(); // Get compass value
    
    send_info_to_web(); // Send sensor values to webpage

    if (color == "red") {
      drive(1, Motor_backward); // Reverse back from red-line
      if (current_degree >= 270 || current_degree <= 90) {
        rotate_vehicle(current_degree - 45);
      } else {
        rotate_vehicle(current_degree + 45);
      }
    } else if (color == "blue") {
      var_speed = blue_speed;
    } else if (color == "green") {
      var_speed = green_speed;
    }

    travel_distance(50, var_speed);

  }
  move_motor(0,0,0,0);
}

//===========================================================Entry========================================
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

  lidar.begin(0, true);
  lidar.configure(0); //change to 1 for short range, high speed

  compass_offset = compass_logic(); // Get compass reading on start-up
   EEPROM.get(ENC_ADDR, saved_enc_value);
}

/**
 * TO-DO:
 * ❌ Create maze function
 *    -✓ Start speed at 50% (if not r,g,b: var_speed  = motor_max_speed * 0.5)
 *    -✓ b: var_speed = motor_max_speed * 0.35
 *    -✓ g: var_speed = motor_max_speed * 0.75
 *    -✓ r: avoid (reverse 1cm -> rotate +- 90?)
 * ✓ Compass calibration at startup
 * ✓ Color calibration done with joystick button
 * ✓ Encoder calibration done from site
 * ✓ Use enc calibration in travel functions
 * ✓ Update webpage to only include:
 *    -✓ Security (login)
 *    -✓ Button "Go" to trigger maze start
 *    -✓ LiDar data
 *    -✓ Compass data
 *    -✓ Color data
 *    -✓ Encoder data (mm/pulse)
 *    -✓ Button to calibrate encoder
 * ❌ Finish maze function
 */
 
void loop(){
  get_rgb_value();
  display_lcd();
  current_degree = compass_logic();
  lidar_dist = get_lidar_distance(num_readings);
 
  if (presses <= 3 && !calibrated) {
    if (Serial2.available() > 0) Serial2.readStringUntil("/n"); // Eats all commands
    cal_col();  // Store calibration color
  } else if (presses == 4) {
    //maze();
    get_commands_from_web();
    //Serial.println("Finitor");
  } else if (presses == 5) {
    presses = 0;
  }

//  get_commands_from_web();
  send_info_to_web();
}
