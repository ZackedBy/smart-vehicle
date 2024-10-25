#include <LiquidCrystal.h>

#define pin_in A0
#define digital_pin 2
int analog_strength = 0;
float voltage = 0;

const int rs = 8, en = 6, d4 = 5, d5 = 4, d6 = 7, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  lcd.begin(20,4);
  pinMode(pin_in, INPUT);
  pinMode(digital_pin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  analog_strength = analogRead(pin_in);
  voltage = analog_strength / 204.6;
  Serial.println("Analog: " + String(analog_strength) + ",V: " + String(voltage));
  if(voltage < 1.0){
    digitalWrite(digital_pin, HIGH);
  } else {
    digitalWrite(digital_pin, LOW);
  }
  lcd.setCursor(0,0);
  lcd.print(voltage);
  delay(1000);
}
