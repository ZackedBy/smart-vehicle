int ledPin = 13; // Пин для светодиода

void setup() 
{
  Serial.begin(9600);       // Serial для связи с ESP32
  pinMode(ledPin, OUTPUT);  // Настраиваем пин светодиода как выход
  digitalWrite(ledPin, LOW);
  Serial.println("Set-Up is done.");
  delay(1000);
}

void loop() {
  if (Serial.available() > 0) 
  {      // Проверяем, есть ли данные для чтения
    char command = Serial.read();    // Читаем данные от ESP32
    Serial.println("Connected to Esp32");
    if (command == '1') 
    {
      digitalWrite(ledPin, HIGH);
      Serial.println("Light is on");
    } 
    else if (command == '0') 
    {
      digitalWrite(ledPin, LOW);
      Serial.println("Light is off");  
    }
  }
}
