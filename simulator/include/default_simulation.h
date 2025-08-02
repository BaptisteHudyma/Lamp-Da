#ifndef LMBD_DEFAULT_SIMULATION_H
#define LMBD_DEFAULT_SIMULATION_H

struct defaultSimulation
{
  float fps = 60.f; // how fast animation is

  float ledSizePx = 24.f;   // square size to represent one LED (in pixels)
  float ledPaddingPx = 4.f; // padding between two LEDs (in pixels)
  float ledOffsetPx = 12.f; // how much LED offsets diagonally (in pixels)

  float fakeXorigin = 0; // start lamp N led to the left
  float fakeXend = 0;    // remove N led from right

  float buttonSize = 40.f; // draw a lamp button of 40 pixels
  float buttonMargin = 15.f; // draw a button color indicator 10 pixels larger

  defaultSimulation() {}
};

#endif
