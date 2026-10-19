#pragma once

#include <stdint.h>

#include "board.hpp"

enum class Motion : uint8_t { Running, Done, Failed };
enum class Activity : uint8_t { Idle, Rotate, DriveMs, DriveCm, Calibrate, Maze };
enum class MazeStep : uint8_t {
  Cruise,
  AfterReverse,
  ReadWest,
  ReadNorth,
  Branch,
  Nudge,
  ReadNw,
  ReadSw,
  Finished
};

// Cooperative motions. loop() calls tick() once; no motion holds the CPU until it finishes.
class Pilot {
 public:
  explicit Pilot(Board& board) : board_(board) {}

  void begin();
  void tick();

 private:
  void dispatch(const vehicle::Command& command);
  void halt(const char* status);
  void capture_color();
  void start_rotate(int target, bool back_to_maze, MazeStep then);
  void begin_drive_ms(uint8_t direction, int pwm, uint32_t budget_ms, MazeStep then);
  void start_drive_cm(int distance_cm);
  void begin_maze();
  void begin_calibrate();

  Motion service_rotate();
  Motion service_drive_ms();
  Motion service_drive_cm();
  void service_calibrate();
  void service_maze();
  void cruise();
  void after_reverse();
  void branch();
  bool take_lidar(int* centimeters);
  bool scanning() const;

  Board& board_;
  Activity activity_ = Activity::Idle;
  MazeStep maze_step_ = MazeStep::Cruise;

  bool rotate_armed_ = false;
  bool rotate_back_to_maze_ = false;
  bool rotate_clockwise_ = true;
  int rotate_target_ = 0;
  MazeStep rotate_then_ = MazeStep::Cruise;
  uint32_t rotate_started_ms_ = 0;
  uint32_t rotate_mark_ms_ = 0;
  int rotate_mark_heading_ = 0;

  bool drive_armed_ = false;
  uint8_t drive_dir_ = cfg::kForward;
  int drive_pwm_ = 0;
  uint32_t drive_budget_ms_ = 0;
  uint32_t drive_started_ms_ = 0;
  uint32_t drive_target_pulses_ = 0;
  MazeStep drive_then_ = MazeStep::Cruise;

  bool cal_have_start_ = false;
  int cal_start_cm_ = 0;
  uint32_t cal_started_ms_ = 0;

  bool scan_armed_ = false;
  uint32_t scan_started_ms_ = 0;
  int maze_pwm_ = cfg::kMazeStartPwm;
  int west_cm_ = 0;
  int north_cm_ = 0;
  int nw_cm_ = 0;
  int sw_cm_ = 0;
};
