#ifndef LIDAR_H
#define LIDAR_H

#include <Arduino.h>
#include <LIDARLite.h>

class LidarReader {
    public:
        void begin(LIDARLite& lidar_ref);
        void start(int readings, int interval_ms = 50);
        void update();
        bool ready();
        int get();

    private:
        LIDARLite* lidar;
        int sum = 0;
        int count = 0;
        int target = 0;
        unsigned long last_time = 0;
        int interval = 50;
        bool active = false;
        int result = 0;
};

extern LidarReader lidarReader;

#endif