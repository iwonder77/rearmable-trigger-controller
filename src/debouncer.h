/**
 * @file debouncer.h
 * @brief Turns a noisy on/off signal (such as a mechanical button) into a clean
 *        "just pressed" event.
 *
 * This class is pure input timing logic, it never touches a GPIO pin or reads
 * the clock, the caller reads the hardware and passes in the raw value as well
 * as the current tick. This keeps it reusable for any on/off input.
 */
#pragma once

#include <cstdint>

/**
 * @brief Debounces one on/off input and reports the moment it turns on.
 *
 * Mechanical contacts bounce for a few milliseconds after a press or release,
 * the raw reading flickers before it settles. This class treats each raw
 * reading as a potential yet undefined claim, and only accepts it as a stable
 * claim once it has held steady for the debounce time (and only once per
 * event).
 *
 * - Every change in the raw reading restarts a stopwatch, because it might be
 *   a bounce
 * - When the raw reading has been steady for the debounce time AND differs from
 *   the stable state, it becomes the new stable state
 * - update() returns true only on the one pass where the stable state changes
 * from false to true. That makes it an event ("was just pressed"), not a state
 * ("is pressed"), so a held button produces exactly one event.
 *
 * Typical use case (once per pass of the main loop):
 * @code
 * Debouncer button(20);            // 20 ms debounce window
 * button.reset(raw_reading, now);  // reset after pin is set up
 *
 * while (true) {
 *  if (button.update(raw, now)) {
 *    // the button was just pressed, take action here
 *  }
 * }
 * @endcode
 */
class Debouncer {
public:
  /**
   * @brief Created a debouncer object with the given debounce window
   *
   * @param debounce_ms How long, in milliseconds, the raw reading must stay
   *        unchanged before it is accepted. About 20ms suits a push button, a
   *        longer window filters more noise but adds delay, and taps shorter
   * than the window are ignored
   */
  explicit Debouncer(uint32_t debounce_ms) : debounce_time_ms_(debounce_ms) {}

  /**
   * @brief Sets the starting state from a real reading
   *
   * Call once at startup, AFTER the input pin is configured so that the stable
   * state starts out matching the real input. A button held at power-up counts
   * as "already pressed" rather than a new press, so nothing triggers until it
   * is released and pressed again.
   *
   * @param raw Current raw reading (true = active).
   * @param now Current time in milliseconds.
   */
  void reset(bool raw, uint32_t now) {
    last_raw_ = raw;
    stable_ = raw;
    last_raw_change_ms_ = now;
  }

  /**
   * @brief Feeds in one raw reading and reports whether a new press just
   * happened.
   *
   * Call exactly once per pass of the main loop, in every state. Presses that
   * the application ignores (for example during playback) still need to pass
   * through here so that they are used up immediately instead of being reported
   * later.
   *
   * @param raw Raw reading for this pass (true = active).
   * @param now Current timestamp in milliseconds. Use the same value for every
   * decision made in one pass.
   * @return true only on the pass where the stable state changes to true.
   *         Returns false on every other pass, including when the stable state
   * changes back to false
   *
   * Note: if we also wanted to detect releases, we'd have to define 3 states,
   * an idle state, a just pressed state, and a just released state, and modify
   * this method to return those states accordingly
   */
  bool update(bool raw, uint32_t now) {
    if (raw != last_raw_) {
      // change just occurred (could be a press or bounce), record
      // timestamp now but don't report anything yet
      last_raw_change_ms_ = now;
      last_raw_ = raw;
      return false;
    }
    // use unsigned subtraction to give correct elapsed time even after the
    // millisecond counter wraps around
    if (now - last_raw_change_ms_ < debounce_time_ms_) {
      return false;
    }
    if (raw == stable_) {
      // already the accepted state, nothing new to report (we are only
      // reporting the moment of change, not a continuous reading)
      return false;
    }
    stable_ = raw;
    return stable_; // true for a new presse, false for a new release
  }

  /**
   * @brief Getter for stable state (accepted and debounced state).
   *
   * Use this when you need the current level ("is the input active
   * right now?") rather than the moment it changed.
   */
  bool get_stable_reading() const { return stable_; }

private:
  uint32_t
      debounce_time_ms_;  ///< Steady time required before accepting a change.
  bool last_raw_ = false; ///< Raw reading from the previous pass.
  bool stable_ = false;   ///< Accepted, debounced state.
  uint32_t last_raw_change_ms_ =
      0; ///< When the raw reading last changed (start of stopwatch).
};
