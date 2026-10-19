# Checklist for Presentation

This checklist describes the lab demo. The program to flash is `firmware/`, not the sketches next to this file.

### **General guidelines:**
- [x] **Build quality:** The car does not have any loose/hanging parts and does not fall over easily,
- [x] **Wiring quality:** The wiring is done neatly, there is next to no extra length on the wires, and they don't protrude upwards in a chaotic manner,
- [x] **Motor Calibration:** The car moves in a straight line without noticeable drift towards right or left (test drive 50cm straight with maximum drift of 5cm),
- [x] **Code Integration:** All necessary code for Arduino Mega is combined into one program which runs without errors,
- [x] **Mode Switching:** The car can switch between joystick and ESP-controlled mode using a button.

### **Compass:**
- [x] **Calibration & Accuracy:** The initial compass reading is recorded and used to calculate the offset, which is then applied to readings (North points to true North, etc...)

### **Joystick Control:**
- [x] **Drive:** The car can be driven with the joystick and the direction and speed change based on joystick movement.

### **Webpage Control:**
- [x] **Moving:** The car can be driven forwards and backwards with commands from the webpage,
- [x] **Compass-based Turning:** The car turns accurately when given the commands from the webpage (within a ±5° tolerance),
- [x] **Find North Button:** The car rotates to face North when button on webpage is pressed.

### **LCD Display:**
- [x] **Mode:** Displays the current mode (ESP/Joystick),
- [x] **Compass:** For both modes, display compass degree and direction (N, NE, E, etc.), and pulse counts and distances(cm) travelled for both wheels,
- [x] **Joystick Values:** Displays analog values for both joystick axes (only in joystick mode),
- [x] **ESP Commands:** Displays the latest command given from ESP (only in ESP mode)

### **Code Quality:**
- [x] **Code Cleanup:** All necessary code is combined into one functioning program, unnecessary code is remove, the code is well-organised and descriptive naming is used,
- [x] **Code Structure and Documentation:** The code is divided into separate functions and key parts of the code are commented,
- [x] **Error Handling:** Implemented one of the options or one of similar level. Error is somehow displayed on the LCD.

