#include "MicroPhone.h"

#include <PDM.h>

#include <algorithm>
#include <cstdint>

#include "../ext/noise.h"
#include "../ext/random8.h"
#include "../utils/constants.h"
#include "fft.h"

namespace microphone {

// buffer to read samples into, each sample is 16-bits
constexpr size_t sampleSize = 512;  // samplesFFT;
int16_t _sampleBuffer[sampleSize];
volatile int samplesRead;

uint32_t lastMeasurmentMicros;
uint32_t lastMeasurmentDurationMicros;
void on_PDM_data() {
  const uint32_t newTime = micros();
  lastMeasurmentDurationMicros = newTime - lastMeasurmentMicros;
  lastMeasurmentMicros = newTime;
  // query the number of bytes available
  const int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read((char*)&_sampleBuffer[0], bytesAvailable);

  // number of samples read
  samplesRead = bytesAvailable / 2;
}

static uint32_t lastMicFunctionCall = 0;
bool isStarted = false;

void enable() {
  lastMicFunctionCall = millis();
  if (isStarted) {
    return;
  }

  digitalWrite(PIN_PDM_PWR, HIGH);

  PDM.setBufferSize(512);
  PDM.onReceive(on_PDM_data);

  // initialize PDM with:
  // - one channel (mono mode)
  // - a sample rate
  if (!PDM.begin(1, SAMPLE_RATE)) {
    // block program execution
    while (1) {
      delay(1000);
    }
  }

  // optionally set the gain, defaults to 20
  PDM.setGain(30);

  micDataReal = 0.0f;
  volumeRaw = 0;
  volumeSmth = 0;
  sampleAgc = 0;
  sampleAvg = 0;
  sampleRaw = 0;
  rawSampleAgc = 0;
  my_magnitude = 0;
  FFT_Magnitude = 0;
  FFT_MajorPeak = 1;
  multAgc = 1;
  // reset FFT data
  memset(fftCalc, 0, sizeof(fftCalc));
  memset(fftAvg, 0, sizeof(fftAvg));
  memset(fftResult, 0, sizeof(fftResult));
  for (int i = 0; i < NUM_GEQ_CHANNELS; i += 2)
    fftResult[i] = 16;  // make a tiny pattern
  inputLevel = 128;     // reset level slider to default
  autoResetPeak();

  isStarted = true;
}

void disable() {
  if (!isStarted) {
    return;
  }

  PDM.end();

  digitalWrite(PIN_PDM_PWR, LOW);
  isStarted = false;
}

void disable_after_non_use() {
  if (isStarted and (millis() - lastMicFunctionCall > 1000.0)) {
    // disable microphone if last reading is old
    disable();
  }
}

float get_sound_level_Db() {
  enable();

  static float lastValue = 0;

  if (!samplesRead) return lastValue;

  float sumOfAll = 0.0;
  for (int i = 0; i < samplesRead; i++) {
    sumOfAll += powf(_sampleBuffer[i] / (float)1024.0, 2.0);
  }
  const float average = sumOfAll / (float)samplesRead;

  lastValue = 20.0 * log10f(sqrtf(average));
  // convert to decibels
  return lastValue;
}

bool processFFT(const bool runFFT = true) {
  enable();

  if (samplesRead <= 0) {
    return false;
  }

  // get data
  uint32_t userloopDelay = LOOP_UPDATE_PERIOD;
  for (int i = 0; i < samplesRead; i++) {
    float sample = (_sampleBuffer[i] & 0xFFFF);

    processSample(sample);
    agcAvg();

    vReal[i] = sampleRaw;
    vImag[i] = 0;
  }
  for (int i = samplesRead; i < samplesFFT; i++) {
    vReal[i] = 0;
    vImag[i] = 0;
  }

  volumeSmth = (soundAgc) ? sampleAgc : sampleAvg;
  volumeRaw = (soundAgc) ? rawSampleAgc : sampleRaw;
  // update FFTMagnitude, taking into account AGC amplification
  my_magnitude = FFT_Magnitude;  // / 16.0f, 8.0f, 4.0f done in effects
  if (soundAgc) my_magnitude *= multAgc;
  if (volumeSmth < 1) my_magnitude = 0.001f;  // noise gate closed - mute
  limitSampleDynamics();
  autoResetPeak();

  samplesRead = 0;
  if (runFFT) FFTcode();

  return true;
}

<<<<<<< Updated upstream
}  // namespace microphone
=======
}  // namespace microphone
>>>>>>> Stashed changes
