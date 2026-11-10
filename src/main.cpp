/**
 * @file main.cpp
 * @brief debounced button input to trigger output (LED) with timed lockout
 *
 * A debounced button press starts a timed "playback" output which runs for
 * PLAYBACK_DURATION_MS. For now the status LED stands in for the real trigger
 * outputs (BrightSign and/or sound module). Presses during playback are
 * ignored (no BUSY line implemented here) and a held button triggeres only
 * once: it has to be released and pressed again (rearm).
 *
 * Every pass of the main loop follows the same order, take in the current
 * time and raw button reading, pass it to the debouncer, and run the state
 * machine based on the event returned.
 */
#include <cstdint>
#include <cstdio>

#include "pico/stdlib.h"

#include "debouncer.h"

// ---------- CONFIGURATION -------------------------
constexpr uint STATUS_LED_PIN = 25; // generic Pico onboard LED, on the
                                    // Pro Micro, GPIO25 drives a WS2812,
                                    // so use an external LED on another pin
constexpr uint BUTTON_PIN = 22;     // Buttons are ACTIVE LOW, external pull-up
                                    // circuit is assumed here

constexpr uint32_t DEBOUNCE_MS = 20;
constexpr uint32_t PLAYBACK_DURATION_MS = 5000;
// --------------------------------------------------

// ---------- HARDWARE ------------------------------
static void init_pins() {
  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);

  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  // the RP2040 starts every pin with its internal pull-down enabled, and
  // gpio_init() leaves it that way, turn it off here so it doesn't fight
  // our external pull-up circuit:
  gpio_disable_pulls(BUTTON_PIN);
}

static void set_led(bool on) { gpio_put(STATUS_LED_PIN, on); }

// the button reads 0 (LOW) when pressed, inverting it here allows the
// rest of the code to treat TRUE as "pressed."
static bool button_is_pressed() { return !gpio_get(BUTTON_PIN); }

static uint32_t now_ms() { return to_ms_since_boot(get_absolute_time()); }
// --------------------------------------------------

enum class PlaybackState : uint8_t { IDLE, PLAYING };

int main(void) {
  // begins UART and USB serial for printf(), keep USB enabled even if all the
  // logging is removed since it is what lets `picotool load -f` reboot the
  // board into BOOTSEL mode for reuploading firmware
  stdio_init_all();
  init_pins();

  PlaybackState state = PlaybackState::IDLE;
  uint32_t playback_started_at_ms = 0;

  Debouncer button(DEBOUNCE_MS);
  button.reset(button_is_pressed(), now_ms());

  while (true) {
    // read the time once on every loop, so every decision in this pass uses
    // the same value
    const uint32_t now = now_ms();

    // update the debouncer on every pass, in every state (see below)
    const bool raw = button_is_pressed();
    const bool press_event = button.update(raw, now);

    switch (state) {
    case PlaybackState::IDLE:
      if (press_event) {
        state = PlaybackState::PLAYING;
        playback_started_at_ms = now;
        set_led(true);
        printf("button pressed! starting playback\n");
      }
      break;
    case PlaybackState::PLAYING:
      // call update only for logging, presses are ignored and not acted upon
      if (press_event) {
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
