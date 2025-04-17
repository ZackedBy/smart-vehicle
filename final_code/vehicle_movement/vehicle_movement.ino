#include <LiquidCrystal.h>
#include <Wire.h>
#include <LIDARLite.h>
#include "Adafruit_TCS34725.h"
#include <EEPROM.h>

// JoyStick Press Variables
#define joy_key 18
const int debounce_time = 500;
volatile unsigned long last_button_press_time = 0;

// Motor Variables
#define Motor_L_dir_pin 7
#define Motor_R_dir_pin 8
#define Motor_L_pwm_pin 9
#define Motor_R_pwm_pin 10
const byte Motor_forward = 0, Motor_backward = 1;
const int motor_max_speed = 255;

// Encoder Variables
#define ENC_A_R 2
#define ENC_A_L 3
#define ENC_B_R 11
#define ENC_B_L 12
volatile int pulse_count_R = 0, pulse_count_L = 0;
const byte ENC_ADDR = 0x00;
float saved_enc_value;

// LCD Variables
#define rs 43
#define en 39
#define d4 29
#define d5 27
#define d6 25
#define d7 23
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned long prev_millis_lcd = 0;

// Compass Variables
const uint8_t CMPS14_address = 0x60;
int current_degree = 0, prev_checked_degree = 0;
int compass_offset;
unsigned long prev_error_check = 0;

// LiDAR Variables
LIDARLite lidar;
int lidar_dist = 0;       // Average distance after x amount of readings
const int num_readings = 20;  // X amount of readings

//RGB Sensor Variables
Adafruit_TCS34725 rgb_sens = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X);
float r, g, b;
String color = "black";
struct Color {
  int r,g,b;
  String toString() {
    return "R: " + String(r) + ", G: " + String(g) + ", B: " + String(b);
  }
};
Color calibratedColors[3];
volatile int presses = 0;
volatile boolean calibrated = true;
#define NUM_COLORS (sizeof(calibratedColors) / sizeof(calibratedColors[0]))

// Website Variables
String message; //whole message sent from website

//==================================================================LiDAR Functions=============================================

// Reads the distance from Lidar
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

// Enables user to calibrate the RGB sensor
void cal_col() {
  rgb_sens.getRGB(&r, &g, &b);
  if (presses >= 1 && presses <= NUM_COLORS) {
    calibratedColors[presses - 1] = {int(r), int(g), int(b)};
    Serial.print("Calibrated Color ");
    Serial.print(presses);
    Serial.print(": ");
    Serial.println(calibratedColors[presses - 1].toString());
    calibrated = true;
  }
}

// Checks stored values against colors calibrated with threshold
bool isColorMatch(Color target, int threshold = 10) {
  return (abs(int(r) - target.r) <= threshold) &&
         (abs(int(g) - target.g) <= threshold) &&
         (abs(int(b) - target.b) <= threshold);
}

// Gets each color value in range of 0-255 and then assigns color based on them
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


//==================================================================LCD Specific Functions=============================================

// Displays and refreshes LCD with non-disruptive delay
void display_lcd(){
  if (millis() - prev_millis_lcd >= 500) {
    prev_millis_lcd = millis();
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print(get_direction(current_degree)); lcd.print(": "); lcd.print(current_degree);
    lcd.print("| Lidar: "); lcd.print(lidar_dist);
    
    
    lcd.setCursor(0,1);
    lcd.print(color);
    lcd.print(" | ");
    lcd.print(int(r)); lcd.print(" ");
    lcd.print(int(g)); lcd.print(" ");
    lcd.print(int(b));

    lcd.setCursor(0,2);
    lcd.print("8==================D");

    lcd.setCursor(0,3);
    lcd.print("mm/p: ");
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

void get_commands_from_web() {
  if(Serial2.available() > 0) {

    message = Serial2.readStringUntil('\n');
    message.trim();
    message.toLowerCase();
    Serial.println(message);

    if (message.startsWith("maze")) {
      doMaze();
    } else if (message.startsWith("calibrate")) {
      cal_enc();
    } else {
      Serial.println("Command not found: " + message);
    }
  }
}

// Sends necessary data to ESP
void send_info_to_ESP(){
  Serial2.println("LIDAR:" + String(lidar_dist));
  Serial2.println("COMP:" + String(current_degree));
  Serial2.println("RGB:" + color);
}

// Calibrates the encoder
void cal_enc() {
  //rotate_vehicle(0);
  reset_dist_variables();

  //float cm_per_enc_pulse = 0.0;
  volatile int pulse_avg = 0;
  int start_lidar_dist = get_lidar_distance(20);
  int curr_lidar_dist = start_lidar_dist;
  const int distance_to_travel = 20;
  Serial.println("Start: " + String(start_lidar_dist));
  move_motor(Motor_forward, Motor_forward, 122, 122);
  while(true) {
    curr_lidar_dist = get_lidar_distance(5);
    Serial.println("Curry dist: " + String(curr_lidar_dist));
    if (abs(start_lidar_dist - curr_lidar_dist) >= distance_to_travel) {
      //save to EEPROM
      pulse_avg = (pulse_count_L + pulse_count_R) / 2;
      Serial.println("Pulse avg: " + String(pulse_avg));
      saved_enc_value = (float(distance_to_travel) / pulse_avg) * 100;
      Serial.print("MM PERRY PULSY: ");
      Serial.println(saved_enc_value, 4);
      EEPROM.put(ENC_ADDR, saved_enc_value);
      break;
    }
  }
  EEPROM.get(ENC_ADDR, saved_enc_value);
  Serial2.println("ENC:" + String(saved_enc_value));
  move_motor(0, 0, 0, 0);
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
    send_info_to_ESP();
    display_lcd();

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

// Activates if lidar detects blockage
boolean check_path_and_turn(){
  move_motor(0, 0, 0, 0);

  rotate_vehicle(270);
  int w = get_lidar_distance(num_readings);
  Serial.print("W: ");
  Serial.print(w);
  rotate_vehicle(0); // change to 180 if end point is in south
  int n = get_lidar_distance(num_readings);
  Serial.print("\tN: ");
  Serial.println(n);
  int lim = 20;

  boolean finish = false;
  
  if (w < lim && n > lim) rotate_vehicle(0);
  else if (w < lim && n < lim) {
        rotate_vehicle(90);
        drive(10, Motor_forward);
        finish = true;    
  } else rotate_vehicle(270); 

  return finish;
}


// Drive without error checking
void drive(int seconds, byte dir) {
  int ms = seconds * 100;
  move_motor(dir, dir, motor_max_speed, motor_max_speed);
  delay(ms);
  move_motor(0, 0, 0, 0);
}

//==========================================================Interrupt Functions=============================================

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
    message = "";
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
}

// Have a guess ya dope xoxo ;)
void doMaze() {
  int start_speed = (int) motor_max_speed * 0.5;
  int blue_speed = (int) motor_max_speed * 0.35;
  int green_speed = (int) motor_max_speed * 0.75;
  int var_speed = start_speed;

  reset_dist_variables();
  rotate_vehicle(270);
  
  while(true) {
    move_motor(Motor_forward, Motor_forward, var_speed, var_speed);
    current_degree = compass_logic(); // Get compass value
    Serial.print("Deg: ");
    Serial.print(current_degree);
    Serial.print("\tLidar: ");
    Serial.println(lidar_dist);
    
    get_rgb_value(); // Get color from RGB sensor
  
    if (color == "red") {
      drive(8, Motor_backward); // Reverse back from red-line
      Serial.println("Rotating on red");
      if (current_degree < 180) {
        rotate_vehicle(270);
      } else {
         rotate_vehicle(315);
         int nw = get_lidar_distance(num_readings);
         rotate_vehicle(225);
         int sw = get_lidar_distance(num_readings);
         if (nw > sw) rotate_vehicle(325);
         else rotate_vehicle(215);
      }

    } else if (color == "blue") {
      Serial.println("Hit blue");
      var_speed = blue_speed;
    } else if (color == "green") {
      Serial.println("Hit Green");
      var_speed = green_speed;
    }

    lidar_dist = get_lidar_distance(num_readings); // Get distance value from lidar
    if (lidar_dist <= 10) {
      Serial.println("Rotating on LIDAR");
      bool finish = check_path_and_turn();
      if (finish) break;
    } 
    display_lcd();
    send_info_to_ESP(); // Send sensor values to webpage
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

  lidar.begin(1, true);//change to 1 for short range, high speed

  compass_offset = compass_logic(); // Get compass reading on start-up
  EEPROM.get(ENC_ADDR, saved_enc_value);
  Serial2.println("ENC:" + String(saved_enc_value));
}

void loop(){
  get_rgb_value();
  display_lcd();
  current_degree = compass_logic();
  lidar_dist = get_lidar_distance(num_readings);

  if (presses <= NUM_COLORS && !calibrated) {
    if (Serial2.available() > 0) Serial2.readStringUntil("/n"); // Eats all commands
    cal_col();  // Store calibration color
  } else if (presses == NUM_COLORS + 1) {
    get_commands_from_web();
  } else if (presses == NUM_COLORS + 2) {
    presses = 0;
  }

  send_info_to_ESP();
}
