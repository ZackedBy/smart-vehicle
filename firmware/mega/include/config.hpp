#pragma once

#include <stdint.h>

// Pin map for the Arduino Mega build in schematics/mega.
namespace cfg {

constexpr uint32_t kBaud = 9600;

constexpr int kJoyButton = 18;
constexpr int kMotorLDir = 7;
constexpr int kMotorRDir = 8;
constexpr int kMotorLPwm = 9;
constexpr int kMotorRPwm = 10;
constexpr int kEncARight = 2;
constexpr int kEncALeft = 3;
// Channel B is wired on the bench and left as an input. Distance uses channel A edges.
constexpr int kEncBRight = 11;
constexpr int kEncBLeft = 12;

constexpr int kLcdRs = 43;
constexpr int kLcdEn = 39;
constexpr int kLcdD4 = 29;
constexpr int kLcdD5 = 27;
constexpr int kLcdD6 = 25;
constexpr int kLcdD7 = 23;

constexpr uint8_t kCompassAddress = 0x60;
constexpr int kEepromAddress = 0;
constexpr uint32_t kEncoderMagic = 0xE5C0417u;

constexpr int kMotorMaxPwm = 255;
constexpr uint8_t kForward = 0;
constexpr uint8_t kBackward = 1;
constexpr int kManualPwm = 127;

constexpr int kHeadingToleranceDeg = 2;
constexpr int kSlowBandDeg = 10;
constexpr int kStallDeadbandDeg = 2;
constexpr uint32_t kRotateTimeoutMs = 8000;
constexpr uint32_t kStallWindowMs = 1500;
constexpr uint32_t kDriveTimeoutMs = 15000;
constexpr uint32_t kCalibrateTimeoutMs = 15000;
constexpr uint32_t kScanTimeoutMs = 2000;
constexpr int kCalibrateDistanceCm = 20;
constexpr int kObstacleStopCm = 10;
constexpr int kJunctionClearCm = 20;
constexpr int kColorThreshold = 10;
constexpr int kColorCount = 3;
constexpr int kLidarSamples = 20;
constexpr uint32_t kLidarIntervalMs = 10;
constexpr uint32_t kLcdPeriodMs = 500;
constexpr uint32_t kTelemetryPeriodMs = 200;
constexpr uint32_t kRgbPeriodMs = 50;
constexpr uint32_t kButtonDebounceMs = 500;
constexpr int kCompassMissLimit = 5;
constexpr int kMaxMoveCm = 500;

// drive() used to multiply its "seconds" argument by 100.
// drive(8) and drive(10) therefore ran for 800 ms and 1000 ms.
constexpr uint32_t kReverseFromLineMs = 800;
constexpr uint32_t kNudgeMs = 1000;

constexpr int kMazeStartPwm = (kMotorMaxPwm * 50) / 100;
constexpr int kMazeBluePwm = (kMotorMaxPwm * 35) / 100;
constexpr int kMazeGreenPwm = (kMotorMaxPwm * 75) / 100;

constexpr int kWest = 270;
constexpr int kNorth = 0;
constexpr int kEast = 90;
constexpr int kNorthwest = 315;
constexpr int kSouthwest = 225;
constexpr int kAvoidNorth = 325;
constexpr int kAvoidSouth = 215;

}  // namespace cfg
