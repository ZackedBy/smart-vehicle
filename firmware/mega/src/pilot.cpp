#include "pilot.hpp"

#include <Arduino.h>

#include <stdio.h>

#include <vehicle_core/control.hpp>
#include <vehicle_core/encoder.hpp>
#include <vehicle_core/heading.hpp>

void Pilot::begin() {
  if (!board_.rgb_ready()) return;
  if (board_.compass_ready()) board_.set_status("Color 0/3");
}

void Pilot::tick() {
  board_.poll(!scanning());

  if (board_.consume_button() && activity_ == Activity::Idle) capture_color();

  vehicle::Command command{};
  if (board_.read_command(&command)) dispatch(command);

  switch (activity_) {
    case Activity::Idle:
      board_.stop_motors();
      break;
    case Activity::Rotate: {
      Motion motion = service_rotate();
      if (motion == Motion::Running) break;
      if (motion == Motion::Failed) {
        activity_ = Activity::Idle;
        break;
      }
      if (rotate_back_to_maze_) {
        activity_ = Activity::Maze;
        maze_step_ = rotate_then_;
        board_.set_status("Maze");
      } else {
        activity_ = Activity::Idle;
        board_.set_status("Ready");
      }
      break;
    }
    case Activity::DriveMs: {
      Motion motion = service_drive_ms();
      if (motion == Motion::Running) break;
      activity_ = Activity::Maze;
      maze_step_ = drive_then_;
      board_.set_status("Maze");
      break;
    }
    case Activity::DriveCm: {
      Motion motion = service_drive_cm();
      if (motion == Motion::Running) break;
      activity_ = Activity::Idle;
      if (motion == Motion::Done) board_.set_status("Ready");
      break;
    }
    case Activity::Calibrate:
      service_calibrate();
      break;
    case Activity::Maze:
      service_maze();
      break;
  }

  board_.render();
  board_.publish();
}

void Pilot::dispatch(const vehicle::Command& command) {
  switch (command.kind) {
    case vehicle::CommandKind::Stop:
      halt("Stopped");
      break;
    case vehicle::CommandKind::Turn: {
      if (!command.has_value || command.value < 0 || command.value > 360) {
        board_.set_status("Bad angle");
        return;
      }
      if (!board_.compass_ready()) {
        board_.set_status("No compass");
        return;
      }
      int target = command.value == 360 ? 0 : command.value;
      start_rotate(target, false, MazeStep::Cruise);
      board_.set_status("Turn");
      break;
    }
    case vehicle::CommandKind::Move:
      if (!command.has_value || command.value <= 0 || command.value > cfg::kMaxMoveCm) {
        board_.set_status("Bad distance");
        return;
      }
      if (!board_.encoder_ready()) {
        board_.set_status("No encoder cal");
        return;
      }
      start_drive_cm(command.value);
      break;
    case vehicle::CommandKind::Maze:
      begin_maze();
      break;
    case vehicle::CommandKind::Calibrate:
      begin_calibrate();
      break;
    case vehicle::CommandKind::Unknown:
      board_.set_status("Bad command");
      break;
    case vehicle::CommandKind::None:
      break;
  }
}

void Pilot::halt(const char* status) {
  board_.stop_motors();
  board_.cancel_lidar_capture();
  rotate_armed_ = false;
  drive_armed_ = false;
  scan_armed_ = false;
  activity_ = Activity::Idle;
  board_.set_status(status);
}

void Pilot::capture_color() {
  if (board_.colors_captured() >= cfg::kColorCount) return;
  board_.capture_color();
  int count = board_.colors_captured();
  if (count >= cfg::kColorCount) {
    board_.set_status("Ready");
    return;
  }
  char text[16];
  snprintf(text, sizeof text, "Color %d/3", count);
  board_.set_status(text);
}

void Pilot::start_rotate(int target, bool back_to_maze, MazeStep then) {
  board_.stop_motors();
  rotate_target_ = vehicle::normalize_heading(target);
  rotate_armed_ = false;
  rotate_back_to_maze_ = back_to_maze;
  rotate_then_ = then;
  scan_armed_ = false;
  activity_ = Activity::Rotate;
}

void Pilot::begin_drive_ms(uint8_t direction, int pwm, uint32_t budget_ms, MazeStep then) {
  board_.stop_motors();
  drive_dir_ = direction;
  drive_pwm_ = pwm;
  drive_budget_ms_ = budget_ms;
  drive_armed_ = false;
  drive_then_ = then;
  activity_ = Activity::DriveMs;
}

void Pilot::start_drive_cm(int distance_cm) {
  int pulses = vehicle::pulses_for_cm(board_.encoder_scale(), distance_cm);
  if (pulses <= 0) {
    board_.set_status("Bad distance");
    return;
  }
  board_.stop_motors();
  board_.reset_encoders();
  drive_target_pulses_ = static_cast<uint32_t>(pulses);
  drive_armed_ = false;
  activity_ = Activity::DriveCm;
  board_.set_status("Drive");
}

void Pilot::begin_maze() {
  maze_pwm_ = cfg::kMazeStartPwm;
  board_.set_status("Maze");
  start_rotate(cfg::kWest, true, MazeStep::Cruise);
}

void Pilot::begin_calibrate() {
  board_.stop_motors();
  cal_started_ms_ = millis();
  cal_have_start_ = false;
  activity_ = Activity::Calibrate;
  board_.set_status("Calibrate");
}

Motion Pilot::service_rotate() {
  if (!board_.compass_ready()) {
    board_.stop_motors();
    rotate_armed_ = false;
    board_.set_status("Compass lost");
    return Motion::Failed;
  }

  int diff = vehicle::shortest_diff(board_.heading(), rotate_target_);
  if (!rotate_armed_) {
    rotate_started_ms_ = millis();
    rotate_mark_ms_ = rotate_started_ms_;
    rotate_mark_heading_ = board_.heading();
    rotate_clockwise_ = diff > 0;
    rotate_armed_ = true;
  }
  if (vehicle::within_tolerance(diff, cfg::kHeadingToleranceDeg)) {
    board_.stop_motors();
    rotate_armed_ = false;
    return Motion::Done;
  }

  uint32_t now = millis();
  if (vehicle::time_elapsed(rotate_started_ms_, now, cfg::kRotateTimeoutMs)) {
    board_.stop_motors();
    rotate_armed_ = false;
    board_.set_status("Turn timeout");
    return Motion::Failed;
  }
  if (vehicle::time_elapsed(rotate_mark_ms_, now, cfg::kStallWindowMs)) {
    int progressed = vehicle::shortest_diff(rotate_mark_heading_, board_.heading());
    if (vehicle::within_tolerance(progressed, cfg::kStallDeadbandDeg)) {
      board_.stop_motors();
      rotate_armed_ = false;
      board_.set_status("Turn blocked");
      return Motion::Failed;
    }
    rotate_mark_ms_ = now;
    rotate_mark_heading_ = board_.heading();
  }

  board_.rotate(rotate_clockwise_,
                vehicle::rotation_pwm(diff, cfg::kMotorMaxPwm, cfg::kSlowBandDeg));
  return Motion::Running;
}

Motion Pilot::service_drive_ms() {
  uint32_t now = millis();
  if (!drive_armed_) {
    drive_started_ms_ = now;
    drive_armed_ = true;
  }
  if (vehicle::time_elapsed(drive_started_ms_, now, drive_budget_ms_)) {
    board_.stop_motors();
    drive_armed_ = false;
    return Motion::Done;
  }
  board_.drive(drive_dir_, drive_pwm_);
  return Motion::Running;
}

Motion Pilot::service_drive_cm() {
  uint32_t now = millis();
  if (!drive_armed_) {
    drive_started_ms_ = now;
    drive_armed_ = true;
    board_.reset_encoders();
  }
  if (vehicle::time_elapsed(drive_started_ms_, now, cfg::kDriveTimeoutMs)) {
    board_.stop_motors();
    drive_armed_ = false;
    board_.set_status("Drive timeout");
    return Motion::Failed;
  }
  if (board_.lidar_ready() && board_.lidar_cm() <= cfg::kObstacleStopCm) {
    board_.stop_motors();
    drive_armed_ = false;
    board_.set_status("Blocked");
    return Motion::Failed;
  }
  if (board_.average_pulses() >= drive_target_pulses_) {
    board_.stop_motors();
    drive_armed_ = false;
    return Motion::Done;
  }
  board_.drive(cfg::kForward, cfg::kManualPwm);
  return Motion::Running;
}

void Pilot::service_calibrate() {
  if (vehicle::time_elapsed(cal_started_ms_, millis(), cfg::kCalibrateTimeoutMs)) {
    halt("Cal timeout");
    return;
  }
  if (!board_.lidar_ready()) {
    board_.set_status("Cal wait lidar");
    return;
  }
  if (!cal_have_start_) {
    cal_start_cm_ = board_.lidar_cm();
    cal_have_start_ = true;
    board_.reset_encoders();
    board_.set_status("Cal drive");
  }

  int delta = board_.lidar_cm() - cal_start_cm_;
  if (delta < 0) delta = -delta;
  board_.drive(cfg::kForward, cfg::kMotorMaxPwm);
  if (delta < cfg::kCalibrateDistanceCm) return;

  board_.stop_motors();
  uint32_t pulses = board_.average_pulses();
  if (pulses > 100000u) pulses = 100000u;
  vehicle::EncoderScale scale =
      vehicle::mm_per_pulse(cfg::kCalibrateDistanceCm, static_cast<int>(pulses));
  if (!scale.ok || !board_.save_scale(scale.mm_per_pulse)) {
    halt("Cal failed");
    return;
  }
  activity_ = Activity::Idle;
  board_.set_status("Cal saved");
}

void Pilot::service_maze() {
  switch (maze_step_) {
    case MazeStep::Cruise:
      cruise();
      break;
    case MazeStep::AfterReverse:
      after_reverse();
      break;
    case MazeStep::ReadWest:
      if (take_lidar(&west_cm_)) start_rotate(cfg::kNorth, true, MazeStep::ReadNorth);
      break;
    case MazeStep::ReadNorth:
      if (take_lidar(&north_cm_)) maze_step_ = MazeStep::Branch;
      break;
    case MazeStep::Branch:
      branch();
      break;
    case MazeStep::Nudge:
      begin_drive_ms(cfg::kForward, cfg::kMotorMaxPwm, cfg::kNudgeMs, MazeStep::Finished);
      break;
    case MazeStep::ReadNw:
      if (take_lidar(&nw_cm_)) start_rotate(cfg::kSouthwest, true, MazeStep::ReadSw);
      break;
    case MazeStep::ReadSw:
      if (take_lidar(&sw_cm_)) {
        int target = nw_cm_ > sw_cm_ ? cfg::kAvoidNorth : cfg::kAvoidSouth;
        start_rotate(target, true, MazeStep::Cruise);
      }
      break;
    case MazeStep::Finished:
      halt("Maze done");
      break;
  }
}

void Pilot::cruise() {
  vehicle::ColorName seen = board_.color();
  if (seen == vehicle::ColorName::Red) {
    begin_drive_ms(cfg::kBackward, cfg::kMotorMaxPwm, cfg::kReverseFromLineMs,
                   MazeStep::AfterReverse);
    return;
  }
  if (seen == vehicle::ColorName::Blue) maze_pwm_ = cfg::kMazeBluePwm;
  else if (seen == vehicle::ColorName::Green) maze_pwm_ = cfg::kMazeGreenPwm;

  if (board_.lidar_ready() && board_.lidar_cm() <= cfg::kObstacleStopCm) {
    board_.stop_motors();
    start_rotate(cfg::kWest, true, MazeStep::ReadWest);
    return;
  }
  board_.drive(cfg::kForward, maze_pwm_);
}

void Pilot::after_reverse() {
  if (!board_.compass_ready()) {
    halt("Compass lost");
    return;
  }
  if (board_.heading() < 180) start_rotate(cfg::kWest, true, MazeStep::Cruise);
  else start_rotate(cfg::kNorthwest, true, MazeStep::ReadNw);
}

void Pilot::branch() {
  bool west_blocked = west_cm_ < cfg::kJunctionClearCm;
  bool north_open = north_cm_ > cfg::kJunctionClearCm;
  bool north_blocked = north_cm_ < cfg::kJunctionClearCm;
  if (west_blocked && north_open) start_rotate(cfg::kNorth, true, MazeStep::Cruise);
  else if (west_blocked && north_blocked) start_rotate(cfg::kEast, true, MazeStep::Nudge);
  else start_rotate(cfg::kWest, true, MazeStep::Cruise);
}

bool Pilot::take_lidar(int* centimeters) {
  if (!scan_armed_) {
    scan_started_ms_ = millis();
    scan_armed_ = true;
    board_.cancel_lidar_capture();
  }
  if (vehicle::time_elapsed(scan_started_ms_, millis(), cfg::kScanTimeoutMs)) {
    halt("Lidar lost");
    return false;
  }
  if (!board_.capture_lidar(centimeters)) return false;
  scan_armed_ = false;
  return true;
}

bool Pilot::scanning() const {
  if (activity_ != Activity::Maze) return false;
  return maze_step_ == MazeStep::ReadWest || maze_step_ == MazeStep::ReadNorth ||
         maze_step_ == MazeStep::ReadNw || maze_step_ == MazeStep::ReadSw;
}
