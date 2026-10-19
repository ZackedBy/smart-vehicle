#include "lidar.h"

LidarReader lidarReader;

void LidarReader::begin(LIDARLite& lidar_ref) {
    lidar = &lidar_ref;
}

void LidarReader::start(int readings, int interval_ms) {
    target = readings;
    interval = interval_ms;
    sum = 0;
    count = 0;
    last_time = 0;
    active = true;
}

void LidarReader::update() {
    if(!active || !lidar) return;

    unsigned long now = millis();
    if (last_time == 0 || now - last_time >= interval) {
        int reading = (count == 0) ? lidar->distance(true) : lidar->distance(false);
        sum += reading;
        count++;
        last_time = now;

        if (count >= target) {
            result = sum / target;
            active = false;
        }
    }
}

bool LidarReader::ready() {
    return !active;
}

int LidarReader::get() {
    return result;
}