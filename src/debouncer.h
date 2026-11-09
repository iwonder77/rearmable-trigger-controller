#pragma once

#include <cstdint>

class Debouncer {
public:
  explicit Debouncer(uint32_t debounce_ms) : debounce_time_ms_(debounce_ms) {}

  // Start from a real reading so a button held at power-up is not a press.
  void reset(bool raw, uint32_t now) {
    last_raw_ = raw;
    stable_ = raw;
    last_raw_change_ms_ = now;
  }

  // Returns true only on the pass where the button becomes pressed.
  bool update(bool raw, uint32_t now) {
    if (raw != last_raw_) {
      // change just occurred (could be a press or bounce), record
      // timestamp now
      last_raw_change_ms_ = now;
      last_raw_ = raw;
      return false;
    }
    if (raw == stable_) {
      // nothing new to report (we are only reporting the moment of change,
      // not a continuous reading)
      return false;
    }
    if (now - last_raw_change_ms_ < debounce_time_ms_) {
      return false;
    }
    stable_ = raw;
    return stable_;
  }

  bool get_stable_reading() const { return stable_; }

private:
  uint32_t debounce_time_ms_;
  bool last_raw_ = false;
  bool stable_ = false;
  uint32_t last_raw_change_ms_ = 0;
};
