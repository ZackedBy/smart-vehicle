# Control flow

`flowchart.png` is the lab planning sketch. The program that runs on the vehicle is `firmware/mega`.

`loop()` reads the compass, the RGB sensor, and the LIDAR, takes one UART line, then advances a single motion: idle, turn, timed drive, distance drive, encoder calibration, or maze. A turn stops when the heading is inside tolerance, when the vehicle stops making progress, or when the turn times out. A failed compass read is not treated as north.
