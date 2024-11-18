#pragma once

/*
 * This file allows you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * EEPROM bytes 2750+ are reserved for your custom use case. (if you extend
 * #define EEPSIZE in const.h) bytes 2400+ are currently ununsed, but might be
 * used for future wled features
 */

// WARNING Sound reactive variables that are used by the animations or other
// asynchronous routines must NOT have interim values, but only updated in a
// single calculation. These are:
//
// sample     sampleAvg     sampleAgc       samplePeak
//
// fftBin[]   fftResult[]   FFT_MajorPeak   FFT_Magnitude
//
// Otherwise, the animations may asynchronously read interim values of these
// variables.
//

static unsigned useInputFilter =
    0;  // if >0 , enables a bandpass filter 80Hz-8Khz to remove noise. Applies
        // before FFT.

// hackers corner
// #define SOUND_DYNAMICS_LIMITER        // experimental: define to enable a
// dynamics limiter that avoids "sudden flashes" at onsets. Make some effects
// look more "smooth and fluent"

constexpr int SAMPLE_RATE = 16000;  // Base sample rate in Hz - standard.
                                    // Physical sample time -> 50ms
// constexpr int SAMPLE_RATE = 20480;            // Base sample rate in Hz -
// 20Khz is experimental.    Physical sample time -> 25ms constexpr int
// SAMPLE_RATE = 22050;            // Base sample rate in Hz - 22Khz is a
// standard rate. Physical sample time -> 23ms

uint8_t maxVol = 31;  // (was 10) Reasonable value for constant volume for 'peak
                      // detector', as it won't always trigger
uint8_t binNum = 14;  // (was 8)  Used to select the bin for FFT based beat
                      // detection. 14 = 280Hz

//
// AGC presets
//  Note: in C++, "const" implies "static" - no need to explicitly declare
//  everything as "static const"
//
#define AGC_NUM_PRESETS 3  // AGC currently has 3 presets: normal, vivid, lazy

// Normal, Vivid,    Lazy
const double
    agcSampleDecay[AGC_NUM_PRESETS] =  // decay factor for sampleMax, in case
                                       // the current sample is below sampleMax
    {0.9994, 0.9985, 0.9997};

const float agcZoneLow[AGC_NUM_PRESETS] =  // low volume emergency zone
    {32, 28, 36};
const float agcZoneHigh[AGC_NUM_PRESETS] =  // high volume emergency zone
    {240, 240, 248};
const float agcZoneStop[AGC_NUM_PRESETS] =  // disable AGC integrator if we get
                                            // above this level
    {336, 448, 304};

const float
    agcTarget0[AGC_NUM_PRESETS] =  // first AGC setPoint -> between 40% and 65%
    {112, 144, 164};
const float agcTarget0Up[AGC_NUM_PRESETS] =  // setpoint switching value (a poor
                                             // man's bang-bang)
    {88, 64, 116};
const float agcTarget1[AGC_NUM_PRESETS] =  // second AGC setPoint -> around 85%
    {220, 224, 216};

const double
    agcFollowFast[AGC_NUM_PRESETS] =  // quickly follow setpoint - ~0.15 sec
    {1.0 / 192.0, 1.0 / 128.0, 1.0 / 256.0};
const double
    agcFollowSlow[AGC_NUM_PRESETS] =  // slowly follow setpoint  - ~2-15 secs
    {1.0 / 6144.0, 1.0 / 4096.0, 1.0 / 8192.0};

const double agcControlKp[AGC_NUM_PRESETS] =  // AGC - PI control, proportional
                                              // gain parameter
    {0.6, 1.5, 0.65};
const double agcControlKi[AGC_NUM_PRESETS] =  // AGC - PI control, integral gain
                                              // parameter
    {1.7, 1.85, 1.2};

const float
    agcSampleSmooth[AGC_NUM_PRESETS] =  // smoothing factor for sampleAgc (use
                                        // rawSampleAgc if you want the
                                        // non-smoothed value)
    {1.0 / 12.0, 1.0 / 6.0, 1.0 / 16.0};
//
// AGC presets end
//

// used for AGC
int last_soundAgc = -1;  // used to detect AGC mode change (for resetting AGC
// internal error buffers)
double control_integrated = 0.0;  // persistent across calls to agcAvg();
                                  // "integrator control" = accumulated error

static uint8_t inputLevel = 128;  // UI slider value
double sampleMax =
    0;  // Max sample over a few seconds. Needed for AGC controler.

uint8_t samplePeak = 0;  // Flag for peak. 0 = no peak, 1 = possible peak
                         // (often), 2=sure peak (every 1-2seconds)
uint8_t udpSamplePeak =
    0;  // Set at the same tiem as samplePeak, but reset by transmitAudioData
static int micIn = 0.0;  // Current sample starts with negative values and large
                         // values, which is why it's 16 bit signed
float sampleReal = 0.0;  // "sample" as float, to provide bits that are lost
                         // otherwise. Needed for AGC.
static float tmpSample;  // An interim sample variable used for calculations.
static float sampleAdj;  // Gain adjusted sample value
int rawSampleAgc = 0;    // Our AGC sample - raw
float sampleAgc = 0.0;   // AGC sample, smoothed
uint16_t micData;        // Analog input for FFT
uint16_t micDataSm;      // Smoothed mic data, as it's a bit twitchy
float micDataReal = 0.0;  // future support - this one has the full 24bit MicIn
                          // data - lowest 8bit after decimal point
static unsigned long timeOfPeak = 0;

float multAgc = 1.0;  // sample * multAgc = sampleAgc. Our multiplier
float sampleAvg = 0;  // Smoothed Average
// double beat = 0;                              // beat Detection
static double micLev =
    0.0;  // Used to convert returned value to have '0' as minimum. A leveller

#ifndef SR_SQUELCH
uint8_t soundSquelch =
    10.0;  // squelch value for volume reactive routines (config value)
#else
uint8_t soundSquelch =
    SR_SQUELCH;  // squelch value for volume reactive routines (config value)
#endif

#ifndef SR_GAIN
uint8_t sampleGain = 60;  // sample gain (config value)
#else
uint8_t sampleGain = SR_GAIN;  // sample gain (config value)
#endif

static uint8_t soundAgc = 1;  // Automagic gain control: 0 - none, 1 - normal, 2
                              // - vivid, 3 - lazy (config value)

static float expAdjF;   // Used for exponential filter.
float weighting = 0.2;  // Exponential filter weighting. Will be adjustable in a
                        // future release.

// FFT Variables
constexpr uint16_t samplesFFT =
    512;  // Samples in an FFT batch - This value MUST ALWAYS be a power of 2

float FFT_MajorPeak = 1.0f;
float FFT_Magnitude = 0.0001;

// These are the input and output vectors.  Input vectors receive computed
// results from FFT.
static float vReal[samplesFFT];
static float vImag[samplesFFT];
static float windowWeighingFactors[samplesFFT];
float fftBin[samplesFFT];

// Try and normalize fftBin values to a max of 4096, so that 4096/16 = 256.
// Oh, and bins 0,1,2 are no good, so we'll zero them out.
float fftCalc[16];
int fftResult[16];       // Our calculated result table, which we feed to the
                         // animations.
float fftResultMax[16];  // A table used for testing to determine how our
                         // post-processing is working.
float fftAvg[16];

#define FFTBIN_DOWNSCALE \
  0.65  // scale down FFT results, so we end up at ~128 average

// Table of linearNoise results to be multiplied by soundSquelch in order to
// reduce squelch across fftResult bins.
static int linearNoise[16] = {34, 28, 26, 25, 20, 12, 9, 6,
                              4,  4,  3,  2,  2,  2,  2, 2};

// Table of multiplication factors so that we can even out the frequency
// response.
static float fftResultPink[16] = {1.70, 1.71, 1.73, 1.78, 1.68, 1.56,
                                  1.55, 1.63, 1.79, 1.62, 1.80, 2.06,
                                  2.47, 3.35, 6.83, 9.55};

/* limit sound dynamics by contraining "attack" and "decay" times */
constexpr float bigChange =
    196;  // just a representative number - a large, expected sample value
/* values below will be made user-configurable later */
constexpr float attackTime = 800;  // attack time -> 0.8sec
constexpr float decayTime = 2800;  // decay time  -> 2.8sec

/* This fuctions limits the dynamics of sampleAvg and sampleAgc. It does not
 * affect FFTResult[] or raw samples (sample, rawSampleAgc) */
// effects: Gravimeter, Gravcenter, Gravcentric, Noisefire, Plasmoid,
// Freqpixels, Freqwave, Gravfreq, (2D Swirl, 2D Waverly) experimental, as it
// still has side-effects on AGC - AGC detects "silence" to late (due to long
// decay time) and ditches up the gain multiplier.
void limitSampleDynamics(void) {
  static unsigned long last_time = 0;
  static float last_sampleAvg = 0.0f;
  static float last_sampleAgc = 0.0f;

  long delta_time = millis() - last_time;
  delta_time = constrain(
      delta_time, 1, 1000);  // below 1ms -> 1ms; above 1sec -> sily lil hick-up
  float maxAttack = bigChange * float(delta_time) / attackTime;
  float maxDecay = -bigChange * float(delta_time) / decayTime;
  float deltaSample;

  // non-AGC sample
  if ((attackTime > 0) && (decayTime > 0)) {
    deltaSample = sampleAvg - last_sampleAvg;
    if (deltaSample > maxAttack) deltaSample = maxAttack;
    if (deltaSample < maxDecay) deltaSample = maxDecay;
    sampleAvg = last_sampleAvg + deltaSample;
  }
  // same for AGC sample
  if ((attackTime > 0) && (decayTime > 0)) {
    deltaSample = sampleAgc - last_sampleAgc;
    if (deltaSample > maxAttack) deltaSample = maxAttack;
    if (deltaSample < maxDecay) deltaSample = maxDecay;
    sampleAgc = last_sampleAgc + deltaSample;
  }

  last_sampleAvg = sampleAvg;
  last_sampleAgc = sampleAgc;
  last_time = millis();
}

/*
 * A "PI control" multiplier to automatically adjust sound sensitivity.
 *
 * A few tricks are implemented so that sampleAgc does't only utilize 0% and
 * 100%: 0. don't amplify anything below squelch (but keep previous gain)
 * 1. gain input = maximum signal observed in the last 5-10 seconds
 * 2. we use two setpoints, one at ~60%, and one at ~80% of the maximum signal
 * 3. the amplification depends on signal level:
 *    a) normal zone - very slow adjustment
 *    b) emergency zome (<10% or >90%) - very fast adjustment
 */
void agcAvg() {
  const int AGC_preset =
      (soundAgc > 0) ? (soundAgc - 1)
                     : 0;  // make sure the _compiler_ knows this value will not
                           // change while we are inside the function

  float lastMultAgc = multAgc;          // last multiplier used
  float multAgcTemp = multAgc;          // new multiplier
  float tmpAgc = sampleReal * multAgc;  // what-if amplified signal

  float control_error;  // "control error" input for PI control

  if (last_soundAgc != soundAgc)
    control_integrated = 0.0;  // new preset - reset integrator

  if ((fabsf(sampleReal) < 2.0f) || (sampleMax < 1.0)) {
    // MIC signal is "squelched" - deliver silence
    tmpAgc = 0;
    // we need to "spin down" the intgrated error buffer
    if (fabs(control_integrated) < 0.01)
      control_integrated = 0.0;
    else
      control_integrated *= 0.91;
  } else {
    // compute new setpoint
    if (tmpAgc <= agcTarget0Up[AGC_preset])
      multAgcTemp = agcTarget0[AGC_preset] /
                    sampleMax;  // Make the multiplier so that sampleMax *
                                // multiplier = first setpoint
    else
      multAgcTemp = agcTarget1[AGC_preset] /
                    sampleMax;  // Make the multiplier so that sampleMax *
                                // multiplier = second setpoint
  }
  // limit amplification
  if (multAgcTemp > 32.0f) multAgcTemp = 32.0f;
  if (multAgcTemp < 1.0f / 64.0f) multAgcTemp = 1.0f / 64.0f;

  // compute error terms
  control_error = multAgcTemp - lastMultAgc;

  if (((multAgcTemp > 0.085f) &&
       (multAgcTemp < 6.5f))  // integrator anti-windup by clamping
      && (multAgc * sampleMax <
          agcZoneStop[AGC_preset]))  // integrator ceiling (>140% of max)
    control_integrated += control_error * 0.002 *
                          0.25;  // 2ms = integration time; 0.25 for damping
  else
    control_integrated *= 0.9;  // spin down that beasty integrator

  // apply PI Control
  tmpAgc = sampleReal *
           lastMultAgc;  // check "zone" of the signal using previous gain
  if ((tmpAgc > agcZoneHigh[AGC_preset]) ||
      (tmpAgc <
       soundSquelch + agcZoneLow[AGC_preset])) {  // upper/lower energy zone
    multAgcTemp = lastMultAgc + agcFollowFast[AGC_preset] *
                                    agcControlKp[AGC_preset] * control_error;
    multAgcTemp += agcFollowFast[AGC_preset] * agcControlKi[AGC_preset] *
                   control_integrated;
  } else {  // "normal zone"
    multAgcTemp = lastMultAgc + agcFollowSlow[AGC_preset] *
                                    agcControlKp[AGC_preset] * control_error;
    multAgcTemp += agcFollowSlow[AGC_preset] * agcControlKi[AGC_preset] *
                   control_integrated;
  }

  // limit amplification again - PI controller sometimes "overshoots"
  // multAgcTemp = constrain(multAgcTemp, 0.015625f, 32.0f); // 1/64 <
  // multAgcTemp < 32
  if (multAgcTemp > 32.0f) multAgcTemp = 32.0f;
  if (multAgcTemp < 1.0f / 64.0f) multAgcTemp = 1.0f / 64.0f;

  // NOW finally amplify the signal
  tmpAgc = sampleReal * multAgcTemp;            // apply gain to signal
  if (fabsf(sampleReal) < 2.0f) tmpAgc = 0.0f;  // apply squelch threshold
  // tmpAgc = constrain(tmpAgc, 0, 255);
  if (tmpAgc > 255) tmpAgc = 255.0f;  // limit to 8bit
  if (tmpAgc < 1) tmpAgc = 0.0f;      // just to be sure

  // update global vars ONCE - multAgc, sampleAGC, rawSampleAgc
  multAgc = multAgcTemp;
  rawSampleAgc = 0.8f * tmpAgc + 0.2f * (float)rawSampleAgc;
  // update smoothed AGC sample
  if (fabsf(tmpAgc) < 1.0f)
    sampleAgc = 0.5f * tmpAgc + 0.5f * sampleAgc;  // fast path to zero
  else
    sampleAgc +=
        agcSampleSmooth[AGC_preset] * (tmpAgc - sampleAgc);  // smooth path

  sampleAgc = fabsf(sampleAgc);  // // make sure we have a positive value
  last_soundAgc = soundAgc;
}  // agcAvg()

// post-processing and filtering of MIC sample (micDataReal) from FFTcode()
float processSample(int16_t micIn) {
  const int AGC_preset =
      (soundAgc > 0) ? (soundAgc - 1)
                     : 0;  // make sure the _compiler_ knows this value will not
                           // change while we are inside the function
  static unsigned long lastSoundTime = 0;  // for delaying noise gate
  constexpr long MinTimeSilence = 1600;    // 1600ms "grace time" before closing
                                           // noise gate - to avoid chattering

  // remove remaining DC offset from sound signal
  micLev = ((micLev * 8191.0) + micDataReal) /
           8192.0;  // takes a few seconds to "catch up" with the Mic Input
  // if(micIn < micLev) micLev = ((micLev * 31.0) + micDataReal) / 32.0; //
  // align MicLev to lowest input signal
  if (micDataReal < (micLev - 1.2))
    micLev = ((micLev * 31.0) + micDataReal) /
             32.0;  // align with lowest input, but allow some "overlap" to
                    // stabilize the filter
  micIn -= micLev;  // Let's center it to 0 now

  // Using an exponential filter to smooth out the signal. We'll add controls
  // for this in a future release.
  float micInNoDC = fabsf(micDataReal - micLev);
  expAdjF = weighting * micInNoDC + ((1.0 - weighting) * expAdjF);
  expAdjF = fabsf(expAdjF);  // Now (!) take the absolute value

  // expAdjF = (expAdjF <= soundSquelch) ? 0: expAdjF; // super simple noise
  // gate if ((soundSquelch == 0) && (expAdjF < 0.25f)) expAdjF = 0;
  if ((expAdjF <= soundSquelch) ||
      ((soundSquelch == 0) &&
       (expAdjF < 0.25f))) {  // noise gate with "closing delay"
    if ((millis() - lastSoundTime) > MinTimeSilence) expAdjF = 0.0;
  } else {
    lastSoundTime = millis();
  }

  tmpSample = expAdjF;
  micIn = abs(micIn);  // And get the absolute value of each sample

  sampleAdj = tmpSample * sampleGain / 40 * inputLevel / 128 +
              tmpSample / 16;  // Adjust the gain. with inputLevel adjustment
  sampleReal = tmpSample;

  sampleAdj = fmax(fmin(sampleAdj, 255),
                   0);  // Question: why are we limiting the value to 8 bits ???

  // keep "peak" sample, but decay value if current sample is below peak
  if ((sampleMax < sampleReal) && (sampleReal > 0.5)) {
    sampleMax =
        sampleMax +
        0.5 * (sampleReal - sampleMax);  // new peak - with some filtering
    // if (((maxVol < 6) || (binNum < 9)) && (millis() - timeOfPeak > 80) &&
    // (sampleAvg > 1)) {              // another simple way to detect
    // samplePeak
    if ((millis() - timeOfPeak > 80) && (sampleAvg > 1)) {  // no iffs-n-butts
      samplePeak = 2;
      timeOfPeak = millis();
      udpSamplePeak = 2;
    }
  } else {
    if ((multAgc * sampleMax > agcZoneStop[AGC_preset]) && (soundAgc > 0))
      sampleMax =
          sampleMax +
          0.5 * (sampleReal - sampleMax);  // over AGC Zone - get back quickly
    else
      sampleMax =
          sampleMax * agcSampleDecay[AGC_preset];  // signal to zero --> 5-8sec
  }
  if (sampleMax < 0.5) sampleMax = 0.0;

  sampleAvg = ((sampleAvg * 15.0) + sampleAdj) /
              16.0;              // Smooth it out over the last 16 samples.
  sampleAvg = fabsf(sampleAvg);  // make sure we have a positive value

  // Fixes private class variable compiler error. Unsure if this is the correct
  // way of fixing the root problem. -THATDONFC
  if (millis() - timeOfPeak > 10) {  // Auto-reset of samplePeak after
                                     // a complete frame has passed.
    samplePeak = 0;
    udpSamplePeak = 0;
  }

  // Poor man's beat detection by seeing if sample > Average + some value.
  if ((maxVol > 1) && (binNum > 6) && (fftBin[binNum] > maxVol) &&
      (millis() - timeOfPeak > 100) &&
      (sampleAvg > 1)) {  // This goes through ALL of the 255 bins - but ignores
                          // stupid settings
    //  if (sample > (sampleAvg + maxVol) && millis() > (peakTime + 200)) {
    // Then we got a peak, else we don't. The peak has to time out on its own in
    // order to support UDP sound sync.
    samplePeak = max(uint8_t(1), samplePeak);  // ignore this peak if we already
                                               // have an active "good peak"
    if (samplePeak == 1) timeOfPeak = millis();
    udpSamplePeak = max(uint8_t(1), udpSamplePeak);
  }

  // adapt the sound dynamics
  agcAvg();
  return sampleAdj;
}  // getSample()

////////////////////
// Begin FFT Code //
////////////////////

// using latest ArduinoFFT lib, because it supports float and its much faster!
// lib_deps += https://github.com/kosme/arduinoFFT#develop @ 1.9.2

// The following optimizations seem to be optimal on ESP32 (~1.5ms per FFT run)
// #define FFT_SPEED_OVER_PRECISION     // enables use of reciprocals (1/x etc),
// and an a few other speedups - WLEDMM not faster on ESP32 #define
// FFT_SQRT_APPROXIMATION       // enables "quake3" style inverse sqrt - WLEDMM
// slower on ESP32
#define sqrt(x) \
  sqrtf(x)  // little hack that reduces FFT time by 10-50% on ESP32 (as
            // alternative to FFT_SQRT_APPROXIMATION)
#define sqrt_internal sqrtf  // see https://github.com/kosme/arduinoFFT/pull/83

#include "arduinoFFT.h"

// Create FFT object
static ArduinoFFT<float> FFT = ArduinoFFT<float>(
    vReal, vImag, samplesFFT, SAMPLE_RATE, windowWeighingFactors);

float fftAdd(int from, int to) {
  int i = from;
  float result = 0;
  while (i <= to) {
    result += fftBin[i++];
  }
  return result;
}

// Bandpass filter for PDM microphones
static void runMicFilter(
    uint16_t numSamples,
    float *sampleBuffer) {  // pre-filtering of raw samples (band-pass)
  // band pass filter - can reduce noise floor by a factor of 50
  // downside: frequencies below 60Hz will be ignored

  // low frequency cutoff parameter - see
  // https://dsp.stackexchange.com/questions/40462/exponential-moving-average-cut-off-frequency
  // constexpr float alpha = 0.062f;   // 100Hz
  constexpr float alpha = 0.04883f;  //  80Hz
  // constexpr float alpha = 0.03662f; //  60Hz
  // constexpr float alpha = 0.0225f;  //  40Hz
  //  high frequency cutoff  parameter
  // constexpr float beta1 = 0.75;    //  5Khz
  // constexpr float beta1 = 0.82;    //  7Khz
  constexpr float beta1 = 0.8285;  //  8Khz
  // constexpr float beta1 = 0.85;    // 10Khz

  constexpr float beta2 = (1.0f - beta1) / 2.0;
  static float last_vals[2] = {0.0f};  // FIR high freq cutoff filter
  static float lowfilt = 0.0f;         // IIR low frequency cutoff filter

  for (int i = 0; i < numSamples; i++) {
    // FIR lowpass, to remove high frequency noise
    float highFilteredSample;
    if (i < (numSamples - 1))
      highFilteredSample = beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
                           beta2 * sampleBuffer[i + 1];  // smooth out spikes
    else
      highFilteredSample =
          beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
          beta2 * last_vals[1];  // spcial handling for last sample in array
    last_vals[1] = last_vals[0];
    last_vals[0] = sampleBuffer[i];
    sampleBuffer[i] = highFilteredSample;
    // IIR highpass, to remove low frequency noise
    lowfilt += alpha * (sampleBuffer[i] - lowfilt);
    sampleBuffer[i] = sampleBuffer[i] - lowfilt;
  }
}

// sample smoothing, by using a sliding average FIR highpass filter (first half
// of MicFilter from above)
static void runMicSmoothing(uint16_t numSamples, float *sampleBuffer) {
  constexpr float beta1 = 0.8285;  //  ~8Khz
  constexpr float beta2 =
      (1.0f - beta1) / 2.0;            // note to self: better use biquad ?
  static float last_vals[2] = {0.0f};  // FIR filter buffer

  for (int i = 0; i < numSamples; i++) {
    float highFilteredSample;
    if (i < (numSamples - 1))
      highFilteredSample = beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
                           beta2 * sampleBuffer[i + 1];  // smooth out spikes
    else
      highFilteredSample =
          beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
          beta2 * last_vals[1];  // spcial handling for last sample in array
    last_vals[1] = last_vals[0];
    last_vals[0] = sampleBuffer[i];
    sampleBuffer[i] = highFilteredSample;
  }
}

// a variation of above, with higher cut-off frequency
static void runMicSmoothing_v2(uint16_t numSamples, float *sampleBuffer) {
  constexpr float beta1 = 0.85;  // ~10Khz
  constexpr float beta2 =
      (1.0f - beta1) / 2.0;            // note to self: better use biquad ?
  static float last_vals[2] = {0.0f};  // FIR filter buffer

  for (int i = 0; i < numSamples; i++) {
    float highFilteredSample;
    if (i < (numSamples - 1))
      highFilteredSample = beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
                           beta2 * sampleBuffer[i + 1];  // smooth out spikes
    else
      highFilteredSample =
          beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
          beta2 * last_vals[1];  // spcial handling for last sample in array
    last_vals[1] = last_vals[0];
    last_vals[0] = sampleBuffer[i];
    sampleBuffer[i] = highFilteredSample;
  }
}

// High-Pass filter, 6db per octave
static void runHighFilter6db(const float filter, uint16_t numSamples,
                             float *sampleBuffer) {
  static float lowfilt = 0.0f;  // IIR low frequency cutoff filter
  for (int i = 0; i < numSamples; i++) {
    lowfilt += filter * (sampleBuffer[i] - lowfilt);  // lowpass
    sampleBuffer[i] = sampleBuffer[i] - lowfilt;      // lowpass --> highpass
  }
}

// High-Pass filter, 12db per octave
static void runHighFilter12db(const float filter, uint16_t numSamples,
                              float *sampleBuffer) {
  static float lowfilt1 =
      0.0f;  // IIR low frequency cutoff filter - first pass = 6db
  static float lowfilt2 =
      0.0f;  // IIR low frequency cutoff filter - second pass = 12db
  for (int i = 0; i < numSamples; i++) {
    lowfilt1 += filter * (sampleBuffer[i] - lowfilt1);  // first lowpass 6db
    // lowfilt2 += filter * (lowfilt1 - lowfilt2);     // second lowpass +6db
    // sampleBuffer[i] = sampleBuffer[i] - lowfilt2;   // lowpass --> highpass
    // implementation below has better results, compared to the code above
    float pass1Out =
        sampleBuffer[i] -
        lowfilt1;  // output from first stage (lowpass --> highpass)
    lowfilt2 += filter * (pass1Out - lowfilt2);  // second lowpass +6db
    sampleBuffer[i] = pass1Out - lowfilt2;       // lowpass --> highpass
  }
}

// FFT main code
void FFTcode() {
  // micDataSm = ((micData * 3) + micData)/4;

  // input filters applied before FFT
  if (useInputFilter > 0) {
    // filter parameter - we use constexpr as it does not need any RAM
    // (evaluted at compile time) value = 1 - exp(-2*PI * FFilter / FSample);
    // // FFilter: filter cutoff frequency; FSample: sampling frequency
    constexpr float filter30Hz = 0.01823938f;  // rumbling = 10-25hz
    constexpr float filter70Hz = 0.04204211f;  // mains hum = 50-60hz
    constexpr float filter120Hz =
        0.07098564f;  // bad microphones deliver noise below 120Hz
    constexpr float filter185Hz =
        0.10730882f;  // environmental noise is strongest below 180hz: wind,
                      // engine noise, ...
    switch (useInputFilter) {
      case 1:
        runMicFilter(samplesFFT, vReal);
        break;  // PDM microphone bandpass
      case 2:
        runHighFilter12db(filter30Hz, samplesFFT, vReal);
        break;  // rejects rumbling noise
      case 3:
        runMicSmoothing_v2(
            samplesFFT,
            vReal);  // slightly reduce high frequency noise and artefacts
        runHighFilter12db(filter70Hz, samplesFFT,
                          vReal);  // rejects rumbling + mains hum
        break;
      case 4:
        runMicSmoothing_v2(
            samplesFFT,
            vReal);  // slightly reduce high frequency noise and artefacts
        runHighFilter6db(filter120Hz, samplesFFT,
                         vReal);  // rejects everything below 110Hz
        break;
      case 5:
        runMicSmoothing(samplesFFT,
                        vReal);  // reduce high frequency noise and artefacts
        runHighFilter6db(filter185Hz, samplesFFT,
                         vReal);  // reject low frequency noise
        break;
    }
  }

  // find highest sample in the batch
  const int halfSamplesFFT = samplesFFT / 2;  // samplesFFT divided by 2
  float maxSample1 = 0.0;  // max sample from first half of FFT batch
  float maxSample2 = 0.0;  // max sample from second half of FFT batch
  for (int i = 0; i < samplesFFT; i++) {
    // set imaginary parts to 0
    vImag[i] = 0;
    // pick our  our current mic sample - we take the max value from all
    // samples that go into FFT
    if ((vReal[i] <= (INT16_MAX - 1024)) &&
        (vReal[i] >=
         (INT16_MIN +
          1024)))  // skip extreme values - normally these are artefacts
    {
      if (i <= halfSamplesFFT) {
        if (fabsf(vReal[i]) > maxSample1) maxSample1 = fabsf(vReal[i]);
      } else {
        if (fabsf(vReal[i]) > maxSample2) maxSample2 = fabsf(vReal[i]);
      }
    }
  }
  // release first sample to volume reactive effects
  micDataSm = (uint16_t)maxSample1;
  micDataReal = maxSample1;

  FFT.dcRemoval();  // remove DC offset
  // FFT.windowing(FFTWindow::Flat_top, FFTDirection::Forward);  // Weigh data
  // using "Flat Top" window - better amplitude accuracy
  FFT.windowing(FFTWindow::Blackman_Harris,
                FFTDirection::Forward);  // Weigh data using "Blackman-
                                         // Harris" window - sharp peaks due
                                         // to excellent sideband rejection
  FFT.compute(FFTDirection::Forward);    // Compute FFT
  FFT.complexToMagnitude();              // Compute magnitudes
  //
  // vReal[3 .. 255] contain useful data, each a 20Hz interval (60Hz -
  // 5120Hz). There could be interesting data at bins 0 to 2, but there are
  // too many artifacts.
  //

  FFT.majorPeak(
      &FFT_MajorPeak,
      &FFT_Magnitude);  // let the effects know which freq was most dominant
  FFT_MajorPeak =
      constrain(FFT_MajorPeak, 1.0f,
                5120.0f);  // restrict value to range expected by effects
  FFT_Magnitude = fabsf(FFT_Magnitude);

  for (int i = 0; i < samplesFFT; i++) {  // Values for bins 0 and 1 are WAY too
                                          // large. Might as well start at 3.
    float t = 0.0;
    t = fabsf(vReal[i]);  // just to be sure - values in fft bins should be
                          // positive any way
    t = t / 16.0f;        // Reduce magnitude. Want end result to be linear and
                          // ~4096 max.
    fftBin[i] = t;
  }  // for()

  /* This FFT post processing is a DIY endeavour. What we really need is
   * someone with sound engineering expertise to do a great job here AND most
   * importantly, that the animations look GREAT as a result.
   *
   *
   * Andrew's updated mapping of 256 bins down to the 16 result bins with
   * Sample Freq = 10240, samplesFFT = 512 and some overlap. Based on testing,
   * the lowest/Start frequency is 60 Hz (with bin 3) and a highest/End
   * frequency of 5120 Hz in bin 255. Now, Take the 60Hz and multiply
   * by 1.320367784 to get the next frequency and so on until the end. Then
   * detetermine the bins. End frequency = Start frequency * multiplier ^ 16
   * Multiplier = (End frequency/ Start frequency) ^ 1/16
   * Multiplier = 1.320367784
   */

  //                                               Range
  fftCalc[0] = (fftAdd(3, 4)) / 2;        // 60 - 100
  fftCalc[1] = (fftAdd(4, 5)) / 2;        // 80 - 120
  fftCalc[2] = (fftAdd(5, 7)) / 3;        // 100 - 160
  fftCalc[3] = (fftAdd(7, 9)) / 3;        // 140 - 200
  fftCalc[4] = (fftAdd(9, 12)) / 4;       // 180 - 260
  fftCalc[5] = (fftAdd(12, 16)) / 5;      // 240 - 340
  fftCalc[6] = (fftAdd(16, 21)) / 6;      // 320 - 440
  fftCalc[7] = (fftAdd(21, 28)) / 8;      // 420 - 600
  fftCalc[8] = (fftAdd(28, 37)) / 10;     // 580 - 760
  fftCalc[9] = (fftAdd(37, 48)) / 12;     // 740 - 980
  fftCalc[10] = (fftAdd(48, 64)) / 17;    // 960 - 1300
  fftCalc[11] = (fftAdd(64, 84)) / 21;    // 1280 - 1700
  fftCalc[12] = (fftAdd(84, 111)) / 28;   // 1680 - 2240
  fftCalc[13] = (fftAdd(111, 147)) / 37;  // 2220 - 2960
  fftCalc[14] = (fftAdd(147, 194)) / 48;  // 2940 - 3900
  fftCalc[15] = (fftAdd(194, 255)) / 62;  // 3880 - 5120

  //   Noise supression of fftCalc bins using soundSquelch adjustment for
  //   different input types.
  for (int i = 0; i < 16; i++) {
    fftCalc[i] =
        fftCalc[i] - (float)soundSquelch * (float)linearNoise[i] / 4.0 <= 0
            ? 0
            : fftCalc[i];
  }

  // Adjustment for frequency curves.
  for (int i = 0; i < 16; i++) {
    fftCalc[i] = fftCalc[i] * fftResultPink[i];
    // fftCalc[i] *= FFTBIN_DOWNSCALE;   // correct magnitutude to fit into [0
    // ... 255]
  }

  // Manual linear adjustment of gain using sampleGain adjustment for
  // different input types.
  for (int i = 0; i < 16; i++) {
    if (soundAgc)
      fftCalc[i] = fftCalc[i] * multAgc;
    else
      fftCalc[i] = fftCalc[i] * (float)sampleGain / 40.0 * inputLevel / 128 +
                   (float)fftCalc[i] / 16.0;  // with inputLevel adjustment
  }

  // Now, let's dump it all into fftResult. Need to do this, otherwise other
  // routines might grab fftResult values prematurely.
  for (int i = 0; i < 16; i++) {
    // fftResult[i] = (int)fftCalc[i];
    fftResult[i] = constrain(
        (int)fftCalc[i], 0,
        254);  // question: why do we constrain values to 8bit here ???
    fftAvg[i] = (float)fftResult[i] * .05 + (1 - .05) * fftAvg[i];
  }

  // release second sample to volume reactive effects.
  // Releasing a second sample now effectively doubles the "sample rate"
  micDataSm = (uint16_t)maxSample2;
  micDataReal = maxSample2;

  // Looking for fftResultMax for each bin using Pink Noise
  //      for (int i=0; i<16; i++) {
  //          fftResultMax[i] = ((fftResultMax[i] * 63.0) + fftResult[i])
  //          / 64.0;
  //         Serial.print(fftResultMax[i]*fftResultPink[i]);
  //         Serial.print("\t");
  //        }
  //      Serial.println(" ");
}  // FFTcode()
