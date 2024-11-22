#ifndef LMBD_DEFAULT_SIMULATION_H
#define LMBD_DEFAULT_SIMULATION_H

struct defaultSimulation {
  float fps = 60.f; // how fast animation is

  float ledSizePx = 24.f; // square size to represent one LED (in pixels)
  float ledPaddingPx = 4.f; // padding between two LEDs (in pixels)
  float ledOffsetPx = 12.f; // how much LED offsets diagonally (in pixels)

  float fakeXorigin = 0; // start strip N led to the left
  float fakeXend = 0; // remove N led from right

  defaultSimulation(LampTy& strip) {}

  void loop(LampTy& strip) {}
  void customEventHandler(sf::Event& event) {};

  void button_clicked_default(const uint8_t clicks) { }
  void button_hold_default(const uint8_t clicks,
                           const bool isEndOfHoldEvent,
                           const uint32_t holdDuration) { }

  bool button_clicked_usermode(const uint8_t) { return false; }
  bool button_hold_usermode(const uint8_t, const bool, const uint32_t) { return false; }

  void brightness_update(const uint8_t) { }

  void power_on_sequence() { }
  void power_off_sequence() { }
  void write_parameters() { }
  void read_parameters() { }

  bool should_spawn_thread() { return false; }
  void user_thread() { }
};

#endif
