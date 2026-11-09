/**
 * Button-triggered playback
 *
 * A debounced button press starts a timed playback. The status LED stands in
 * for the real trigger outputs.
 */
#include <cstdint>
#include <cstdio>

#include "pico/stdlib.h"

#include "debouncer.h"

constexpr uint STATUS_LED_PIN = 25;
constexpr uint BUTTON_PIN = 22; // active low, external pull-up

constexpr uint32_t DEBOUNCE_MS = 20;
constexpr uint32_t PLAYBACK_DURATION_MS = 5000;

// ---------- HARDWARE ------------------------------
static void init_pins() {
  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);

  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_disable_pulls(BUTTON_PIN); // external pull-up; the RP2040 resets with
                                  // its internal pull-down enabled
}

static void set_led(bool on) { gpio_put(STATUS_LED_PIN, on); }

static bool button_is_pressed() { return !gpio_get(BUTTON_PIN); }

static uint32_t now_ms() { return to_ms_since_boot(get_absolute_time()); }
// --------------------------------------------------

enum class PlaybackState : uint8_t { IDLE, PLAYING };

int main(void) {
  stdio_init_all();
  init_pins();

  PlaybackState state = PlaybackState::IDLE;
  uint32_t playback_started_at_ms = 0;

  Debouncer button(DEBOUNCE_MS);
  button.reset(button_is_pressed(), now_ms());

  while (true) {
    uint32_t now = now_ms();
    bool raw = button_is_pressed();

    switch (state) {
    case PlaybackState::IDLE:
      if (button.update(raw, now)) {
        state = PlaybackState::PLAYING;
        playback_started_at_ms = now;
        set_led(true);
        printf("button pressed! starting playback\n");
      }
      break;
    case PlaybackState::PLAYING:
      // call update for logging
      if (button.update(raw, now)) {
        printf("button pressed during playback - IGNORED\n");
      }
      if (now - playback_started_at_ms > PLAYBACK_DURATION_MS) {
        state = PlaybackState::IDLE;
        set_led(false);
        printf("playback ended\n");
      }
      break;
    }
  }
}
