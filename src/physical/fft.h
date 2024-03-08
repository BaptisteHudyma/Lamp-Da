#pragma once

#define UM_AUDIOREACTIVE_USE_NEW_FFT

// globals
static uint8_t inputLevel = 128;              // UI slider value
#ifndef SR_GAIN
  uint8_t sampleGain = 60;                    // sample gain (config value)
#else
  uint8_t sampleGain = SR_GAIN;               // sample gain (config value)
#endif
static uint8_t soundAgc = 1;                  // Automagic gain control: 0 - none, 1 - normal, 2 - vivid, 3 - lazy (config value)
static uint8_t audioSyncEnabled = 0;          // bit field: bit 0 - send, bit 1 - receive (config value)

// user settable parameters for limitSoundDynamics()
#ifdef UM_AUDIOREACTIVE_DYNAMICS_LIMITER_OFF
static bool limiterOn = false;                 // bool: enable / disable dynamics limiter
#else
static bool limiterOn = true;
#endif
static uint16_t decayTime = 1400;             // int: decay time in milliseconds.  Default 1.40sec
// user settable options for FFTResult scaling
static uint8_t FFTScalingMode = 3;            // 0 none; 1 optimized logarithmic; 2 optimized linear; 3 optimized square root

static volatile bool disableSoundProcessing = false;      // if true, sound processing (FFT, filters, AGC) will be suspended. "volatile" as its shared between tasks.
static bool useBandPassFilter = false;                    // if true, enables a bandpass filter 80Hz-16Khz to remove noise. Applies before FFT.

// audioreactive variables shared with FFT task
static float    micDataReal = 0.0f;             // MicIn data with full 24bit resolution - lowest 8bit after decimal point
static float    multAgc = 1.0f;                 // sample * multAgc = sampleAgc. Our AGC multiplier
static float    sampleAvg = 0.3f;               // Smoothed Average sample - sampleAvg < 1 means "quiet" (simple noise gate)
static float    sampleAgc = 0.0f;               // Smoothed AGC sample

// peak detection
static bool samplePeak = false;      // Boolean flag for peak - used in effects. Responding routine may reset this flag. Auto-reset after strip.getMinShowDelay()
static uint8_t maxVol = 31;          // Reasonable value for constant volume for 'peak detector', as it won't always trigger (deprecated)
static uint8_t binNum = 8;           // Used to select the bin for FFT based beat detection  (deprecated)
static bool udpSamplePeak = false;   // Boolean flag for peak. Set at the same time as samplePeak, but reset by transmitAudioData
static unsigned long timeOfPeak = 0; // time of last sample peak detection.
static void detectSamplePeak(void);  // peak detection function (needs scaled FFT results in vReal[])
static void autoResetPeak(void);     // peak auto-reset function


////////////////////
// Begin FFT Code //
////////////////////

// some prototypes, to ensure consistent interfaces
static float mapf(float x, float in_min, float in_max, float out_min, float out_max); // map function for float
static float fftAddAvg(int from, int to);   // average of several FFT result bins
void FFTcode();      // audio processing task: read samples, run FFT, fill GEQ channels from FFT results
static void runMicFilter(uint16_t numSamples, float *sampleBuffer);          // pre-filtering of raw samples (band-pass)
static void postProcessFFTResults(bool noiseGateOpen, int numberOfChannels); // post-processing and post-amp of GEQ channels

#define NUM_GEQ_CHANNELS 16                                           // number of frequency channels. Don't change !!

// Table of multiplication factors so that we can even out the frequency response.
static float fftResultPink[NUM_GEQ_CHANNELS] = { 1.70f, 1.71f, 1.73f, 1.78f, 1.68f, 1.56f, 1.55f, 1.63f, 1.79f, 1.62f, 1.80f, 2.06f, 2.47f, 3.35f, 6.83f, 9.55f };

// globals and FFT Output variables shared with animations
static float FFT_MajorPeak = 1.0f;              // FFT: strongest (peak) frequency
static float FFT_Magnitude = 0.0f;              // FFT: volume (magnitude) of peak frequency
static uint8_t fftResult[NUM_GEQ_CHANNELS]= {0};// Our calculated freq. channel result table to be used by effects

// FFT Task variables (filtering and post-processing)
static float   fftCalc[NUM_GEQ_CHANNELS] = {0.0f};                    // Try and normalize fftBin values to a max of 4096, so that 4096/16 = 256.
static float   fftAvg[NUM_GEQ_CHANNELS] = {0.0f};                     // Calculated frequency channel results, with smoothing (used if dynamics limiter is ON)

// audio source parameters and constant
//constexpr double SAMPLE_RATE = 22050;        // Base sample rate in Hz - 22Khz is a standard rate. Physical sample time -> 23ms
constexpr double SAMPLE_RATE = 16000;        // 16kHz - use if FFTtask takes more than 20ms. Physical sample time -> 32ms
//constexpr double SAMPLE_RATE = 20480;        // Base sample rate in Hz - 20Khz is experimental.    Physical sample time -> 25ms
//constexpr double SAMPLE_RATE = 10240;        // Base sample rate in Hz - previous default.         Physical sample time -> 50ms

// FFT Constants
constexpr uint16_t samplesFFT_2 = 512;            // Samples in an FFT batch - This value MUST ALWAYS be a power of 2
constexpr uint16_t samplesFFT = 256;          // meaningfull part of FFT results - only the "lower half" contains useful information.
// the following are observed values, supported by a bit of "educated guessing"
//#define FFT_DOWNSCALE 0.65f                             // 20kHz - downscaling factor for FFT results - "Flat-Top" window @20Khz, old freq channels 
#define FFT_DOWNSCALE 0.46f                             // downscaling factor for FFT results - for "Flat-Top" window @22Khz, new freq channels
#define LOG_256  5.54517744f                            // log(256)

// These are the input and output vectors.  Input vectors receive computed results from FFT.
static float vReal[samplesFFT] = {0.0f};       // FFT sample inputs / freq output -  these are our raw result bins
static float vImag[samplesFFT] = {0.0f};       // imaginary parts
#ifdef UM_AUDIOREACTIVE_USE_NEW_FFT
static float windowWeighingFactors[samplesFFT] = {0.0f};
#endif

// Create FFT object
#ifdef UM_AUDIOREACTIVE_USE_NEW_FFT
  // lib_deps += https://github.com/kosme/arduinoFFT#develop @ 1.9.2
  // these options actually cause slow-downs on all esp32 processors, don't use them.
  // #define FFT_SPEED_OVER_PRECISION     // enables use of reciprocals (1/x etc) - not faster on ESP32
  // #define FFT_SQRT_APPROXIMATION       // enables "quake3" style inverse sqrt  - slower on ESP32
  // Below options are forcing ArduinoFFT to use sqrtf() instead of sqrt()
  #define sqrt(x) sqrtf(x)             // little hack that reduces FFT time by 10-50% on ESP32
#else
  // around 40% slower on -S2
  // lib_deps += https://github.com/blazoncek/arduinoFFT.git
#endif

#include <arduinoFFT.h>

#ifdef UM_AUDIOREACTIVE_USE_NEW_FFT
static ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, samplesFFT, SAMPLE_RATE, windowWeighingFactors);
#else
static arduinoFFT FFT = arduinoFFT(vReal, vImag, samplesFFT, SAMPLE_RATE);
#endif

// Helper functions

// float version of map()
static float mapf(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// compute average of several FFT result bins
static float fftAddAvg(int from, int to) {
  float result = 0.0f;
  for (int i = from; i <= to; i++) {
    result += vReal[i];
  }
  return result / float(to - from + 1);
}

//
// FFT main task
//
void FFTcode()
{
    // band pass filter - can reduce noise floor by a factor of 50
    // downside: frequencies below 100Hz will be ignored
    if (useBandPassFilter) runMicFilter(samplesFFT, vReal);

    // find highest sample in the batch
    float maxSample = 0.0f;                         // max sample from FFT batch
    for (int i=0; i < samplesFFT; i++) {
	    // set imaginary parts to 0
      vImag[i] = 0;
	    // pick our  our current mic sample - we take the max value from all samples that go into FFT
	    if ((vReal[i] <= (INT16_MAX - 1024)) && (vReal[i] >= (INT16_MIN + 1024)))  //skip extreme values - normally these are artefacts
        if (fabsf((float)vReal[i]) > maxSample) maxSample = fabsf((float)vReal[i]);
    }
    // release highest sample to volume reactive effects early - not strictly necessary here - could also be done at the end of the function
    // early release allows the filters (getSample() and agcAvg()) to work with fresh values - we will have matching gain and noise gate values when we want to process the FFT results.
    micDataReal = maxSample;

    if (sampleAvg > 0.25f) { // noise gate open means that FFT results will be used. Don't run FFT if results are not needed.
      // run FFT (takes 3-5ms on ESP32, ~12ms on ESP32-S2)
#ifdef UM_AUDIOREACTIVE_USE_NEW_FFT
      FFT.dcRemoval();                                            // remove DC offset
      FFT.windowing( FFTWindow::Flat_top, FFTDirection::Forward); // Weigh data using "Flat Top" function - better amplitude accuracy
      //FFT.windowing(FFTWindow::Blackman_Harris, FFTDirection::Forward);  // Weigh data using "Blackman- Harris" window - sharp peaks due to excellent sideband rejection
      FFT.compute( FFTDirection::Forward );                       // Compute FFT
      FFT.complexToMagnitude();                                   // Compute magnitudes
#else
      FFT.DCRemoval(); // let FFT lib remove DC component, so we don't need to care about this in getSamples()

      //FFT.Windowing( FFT_WIN_TYP_HAMMING, FFT_FORWARD );        // Weigh data - standard Hamming window
      //FFT.Windowing( FFT_WIN_TYP_BLACKMAN, FFT_FORWARD );       // Blackman window - better side freq rejection
      //FFT.Windowing( FFT_WIN_TYP_BLACKMAN_HARRIS, FFT_FORWARD );// Blackman-Harris - excellent sideband rejection
      FFT.Windowing( FFT_WIN_TYP_FLT_TOP, FFT_FORWARD );          // Flat Top Window - better amplitude accuracy
      FFT.Compute( FFT_FORWARD );                             // Compute FFT
      FFT.ComplexToMagnitude();                               // Compute magnitudes
#endif

      FFT.majorPeak(&FFT_MajorPeak, &FFT_Magnitude);              // let the effects know which freq was most dominant
      FFT_MajorPeak = constrain(FFT_MajorPeak, 1.0f, 11025.0f);   // restrict value to range expected by effects
    } else { // noise gate closed - only clear results as FFT was skipped. MIC samples are still valid when we do this.
      memset(vReal, 0, sizeof(vReal));
      FFT_MajorPeak = 1;
      FFT_Magnitude = 0.001;
    }

    for (int i = 0; i < samplesFFT; i++) {
      float t = fabsf(vReal[i]);                      // just to be sure - values in fft bins should be positive any way
      vReal[i] = t / 16.0f;                           // Reduce magnitude. Want end result to be scaled linear and ~4096 max.
    } // for()

    // mapping of FFT result bins to frequency channels
    if (fabsf(sampleAvg) > 0.5f) { // noise gate open
#if 0
    /* This FFT post processing is a DIY endeavour. What we really need is someone with sound engineering expertise to do a great job here AND most importantly, that the animations look GREAT as a result.
    *
    * Andrew's updated mapping of 256 bins down to the 16 result bins with Sample Freq = 10240, samplesFFT = 512 and some overlap.
    * Based on testing, the lowest/Start frequency is 60 Hz (with bin 3) and a highest/End frequency of 5120 Hz in bin 255.
    * Now, Take the 60Hz and multiply by 1.320367784 to get the next frequency and so on until the end. Then determine the bins.
    * End frequency = Start frequency * multiplier ^ 16
    * Multiplier = (End frequency/ Start frequency) ^ 1/16
    * Multiplier = 1.320367784
    */                                    //  Range
      fftCalc[ 0] = fftAddAvg(2,4);       // 60 - 100
      fftCalc[ 1] = fftAddAvg(4,5);       // 80 - 120
      fftCalc[ 2] = fftAddAvg(5,7);       // 100 - 160
      fftCalc[ 3] = fftAddAvg(7,9);       // 140 - 200
      fftCalc[ 4] = fftAddAvg(9,12);      // 180 - 260
      fftCalc[ 5] = fftAddAvg(12,16);     // 240 - 340
      fftCalc[ 6] = fftAddAvg(16,21);     // 320 - 440
      fftCalc[ 7] = fftAddAvg(21,29);     // 420 - 600
      fftCalc[ 8] = fftAddAvg(29,37);     // 580 - 760
      fftCalc[ 9] = fftAddAvg(37,48);     // 740 - 980
      fftCalc[10] = fftAddAvg(48,64);     // 960 - 1300
      fftCalc[11] = fftAddAvg(64,84);     // 1280 - 1700
      fftCalc[12] = fftAddAvg(84,111);    // 1680 - 2240
      fftCalc[13] = fftAddAvg(111,147);   // 2220 - 2960
      fftCalc[14] = fftAddAvg(147,194);   // 2940 - 3900
      fftCalc[15] = fftAddAvg(194,250);   // 3880 - 5000 // avoid the last 5 bins, which are usually inaccurate
#else
      /* new mapping, optimized for 22050 Hz by softhack007 */
                                                    // bins frequency  range
      if (useBandPassFilter) {
        // skip frequencies below 100hz
        fftCalc[ 0] = 0.8f * fftAddAvg(3,4);
        fftCalc[ 1] = 0.9f * fftAddAvg(4,5);
        fftCalc[ 2] = fftAddAvg(5,6);
        fftCalc[ 3] = fftAddAvg(6,7);
        // don't use the last bins from 206 to 255. 
        fftCalc[15] = fftAddAvg(165,205) * 0.75f;   // 40 7106 - 8828 high             -- with some damping
      } else {
        fftCalc[ 0] = fftAddAvg(1,2);               // 1    43 - 86   sub-bass
        fftCalc[ 1] = fftAddAvg(2,3);               // 1    86 - 129  bass
        fftCalc[ 2] = fftAddAvg(3,5);               // 2   129 - 216  bass
        fftCalc[ 3] = fftAddAvg(5,7);               // 2   216 - 301  bass + midrange
        // don't use the last bins from 216 to 255. They are usually contaminated by aliasing (aka noise) 
        fftCalc[15] = fftAddAvg(165,215) * 0.70f;   // 50 7106 - 9259 high             -- with some damping
      }
      fftCalc[ 4] = fftAddAvg(7,10);                // 3   301 - 430  midrange
      fftCalc[ 5] = fftAddAvg(10,13);               // 3   430 - 560  midrange
      fftCalc[ 6] = fftAddAvg(13,19);               // 5   560 - 818  midrange
      fftCalc[ 7] = fftAddAvg(19,26);               // 7   818 - 1120 midrange -- 1Khz should always be the center !
      fftCalc[ 8] = fftAddAvg(26,33);               // 7  1120 - 1421 midrange
      fftCalc[ 9] = fftAddAvg(33,44);               // 9  1421 - 1895 midrange
      fftCalc[10] = fftAddAvg(44,56);               // 12 1895 - 2412 midrange + high mid
      fftCalc[11] = fftAddAvg(56,70);               // 14 2412 - 3015 high mid
      fftCalc[12] = fftAddAvg(70,86);               // 16 3015 - 3704 high mid
      fftCalc[13] = fftAddAvg(86,104);              // 18 3704 - 4479 high mid
      fftCalc[14] = fftAddAvg(104,165) * 0.88f;     // 61 4479 - 7106 high mid + high  -- with slight damping
#endif
    } else {  // noise gate closed - just decay old values
      for (int i=0; i < NUM_GEQ_CHANNELS; i++) {
        fftCalc[i] *= 0.85f;  // decay to zero
        if (fftCalc[i] < 4.0f) fftCalc[i] = 0.0f;
      }
    }

    // post-processing of frequency channels (pink noise adjustment, AGC, smoothing, scaling)
    postProcessFFTResults((fabsf(sampleAvg) > 0.25f)? true : false , NUM_GEQ_CHANNELS);

    // run peak detection
    autoResetPeak();
    detectSamplePeak();
} // FFTcode() task end


///////////////////////////
// Pre / Postprocessing  //
///////////////////////////

static void runMicFilter(uint16_t numSamples, float *sampleBuffer)          // pre-filtering of raw samples (band-pass)
{
  // low frequency cutoff parameter - see https://dsp.stackexchange.com/questions/40462/exponential-moving-average-cut-off-frequency
  //constexpr float alpha = 0.04f;   // 150Hz
  //constexpr float alpha = 0.03f;   // 110Hz
  constexpr float alpha = 0.0225f; // 80hz
  //constexpr float alpha = 0.01693f;// 60hz
  // high frequency cutoff  parameter
  //constexpr float beta1 = 0.75f;   // 11Khz
  //constexpr float beta1 = 0.82f;   // 15Khz
  //constexpr float beta1 = 0.8285f; // 18Khz
  constexpr float beta1 = 0.85f;  // 20Khz

  constexpr float beta2 = (1.0f - beta1) / 2.0f;
  static float last_vals[2] = { 0.0f }; // FIR high freq cutoff filter
  static float lowfilt = 0.0f;          // IIR low frequency cutoff filter

  for (int i=0; i < numSamples; i++) {
        // FIR lowpass, to remove high frequency noise
        float highFilteredSample;
        if (i < (numSamples-1)) highFilteredSample = beta1*sampleBuffer[i] + beta2*last_vals[0] + beta2*sampleBuffer[i+1];  // smooth out spikes
        else highFilteredSample = beta1*sampleBuffer[i] + beta2*last_vals[0]  + beta2*last_vals[1];                  // special handling for last sample in array
        last_vals[1] = last_vals[0];
        last_vals[0] = sampleBuffer[i];
        sampleBuffer[i] = highFilteredSample;
        // IIR highpass, to remove low frequency noise
        lowfilt += alpha * (sampleBuffer[i] - lowfilt);
        sampleBuffer[i] = sampleBuffer[i] - lowfilt;
  }
}

static void postProcessFFTResults(bool noiseGateOpen, int numberOfChannels) // post-processing and post-amp of GEQ channels
{
    for (int i=0; i < numberOfChannels; i++) {
      
      if (noiseGateOpen) { // noise gate open
        // Adjustment for frequency curves.
        fftCalc[i] *= fftResultPink[i];
        if (FFTScalingMode > 0) fftCalc[i] *= FFT_DOWNSCALE;  // adjustment related to FFT windowing function
        // Manual linear adjustment of gain using sampleGain adjustment for different input types.
        fftCalc[i] *= soundAgc ? multAgc : ((float)sampleGain/40.0f * (float)inputLevel/128.0f + 1.0f/16.0f); //apply gain, with inputLevel adjustment
        if(fftCalc[i] < 0) fftCalc[i] = 0;
      }

      // smooth results - rise fast, fall slower
      if(fftCalc[i] > fftAvg[i])   // rise fast 
        fftAvg[i] = fftCalc[i] *0.75f + 0.25f*fftAvg[i];  // will need approx 2 cycles (50ms) for converging against fftCalc[i]
      else {                       // fall slow
        if (decayTime < 1000) fftAvg[i] = fftCalc[i]*0.22f + 0.78f*fftAvg[i];       // approx  5 cycles (225ms) for falling to zero
        else if (decayTime < 2000) fftAvg[i] = fftCalc[i]*0.17f + 0.83f*fftAvg[i];  // default - approx  9 cycles (225ms) for falling to zero
        else if (decayTime < 3000) fftAvg[i] = fftCalc[i]*0.14f + 0.86f*fftAvg[i];  // approx 14 cycles (350ms) for falling to zero
        else fftAvg[i] = fftCalc[i]*0.1f  + 0.9f*fftAvg[i];                         // approx 20 cycles (500ms) for falling to zero
      }
      // constrain internal vars - just to be sure
      fftCalc[i] = constrain(fftCalc[i], 0.0f, 1023.0f);
      fftAvg[i] = constrain(fftAvg[i], 0.0f, 1023.0f);

      float currentResult;
      if(limiterOn == true)
        currentResult = fftAvg[i];
      else
        currentResult = fftCalc[i];

      switch (FFTScalingMode) {
        case 1:
            // Logarithmic scaling
            currentResult *= 0.42f;                      // 42 is the answer ;-)
            currentResult -= 8.0f;                       // this skips the lowest row, giving some room for peaks
            if (currentResult > 1.0f) currentResult = logf(currentResult); // log to base "e", which is the fastest log() function
            else currentResult = 0.0f;                   // special handling, because log(1) = 0; log(0) = undefined
            currentResult *= 0.85f + (float(i)/18.0f);  // extra up-scaling for high frequencies
            currentResult = mapf(currentResult, 0, LOG_256, 0, 255); // map [log(1) ... log(255)] to [0 ... 255]
        break;
        case 2:
            // Linear scaling
            currentResult *= 0.30f;                     // needs a bit more damping, get stay below 255
            currentResult -= 4.0f;                       // giving a bit more room for peaks
            if (currentResult < 1.0f) currentResult = 0.0f;
            currentResult *= 0.85f + (float(i)/1.8f);   // extra up-scaling for high frequencies
        break;
        case 3:
            // square root scaling
            currentResult *= 0.38f;
            currentResult -= 6.0f;
            if (currentResult > 1.0f) currentResult = sqrtf(currentResult);
            else currentResult = 0.0f;                   // special handling, because sqrt(0) = undefined
            currentResult *= 0.85f + (float(i)/4.5f);   // extra up-scaling for high frequencies
            currentResult = mapf(currentResult, 0.0, 16.0, 0.0, 255.0); // map [sqrt(1) ... sqrt(256)] to [0 ... 255]
        break;

        case 0:
        default:
            // no scaling - leave freq bins as-is
            currentResult -= 4; // just a bit more room for peaks
        break;
      }

      // Now, let's dump it all into fftResult. Need to do this, otherwise other routines might grab fftResult values prematurely.
      if (soundAgc > 0) {  // apply extra "GEQ Gain" if set by user
        float post_gain = (float)inputLevel/128.0f;
        if (post_gain < 1.0f) post_gain = ((post_gain -1.0f) * 0.8f) +1.0f;
        currentResult *= post_gain;
      }
      fftResult[i] = constrain((int)currentResult, 0, 255);
    }
}
////////////////////
// Peak detection //
////////////////////

// peak detection is called from FFT task when vReal[] contains valid FFT results
static void detectSamplePeak(void) {
  bool havePeak = false;
  // softhack007: this code continuously triggers while amplitude in the selected bin is above a certain threshold. So it does not detect peaks - it detects high activity in a frequency bin.
  // Poor man's beat detection by seeing if sample > Average + some value.
  // This goes through ALL of the 255 bins - but ignores stupid settings
  // Then we got a peak, else we don't. The peak has to time out on its own in order to support UDP sound sync.
  if ((sampleAvg > 1) && (maxVol > 0) && (binNum > 4) && (vReal[binNum] > maxVol) && ((millis() - timeOfPeak) > 100)) {
    havePeak = true;
  }

  if (havePeak) {
    samplePeak    = true;
    timeOfPeak    = millis();
    udpSamplePeak = true;
  }
}

static void autoResetPeak(void) {
  uint16_t MinShowDelay = 50;  // Fixes private class variable compiler error. Unsure if this is the correct way of fixing the root problem. -THATDONFC
  if (millis() - timeOfPeak > MinShowDelay) {          // Auto-reset of samplePeak after a complete frame has passed.
    samplePeak = false;
    if (audioSyncEnabled == 0) udpSamplePeak = false;  // this is normally reset by transmitAudioData
  }
}

// used for AGC
int      last_soundAgc = -1;   // used to detect AGC mode change (for resetting AGC internal error buffers)
double   control_integrated = 0.0;   // persistent across calls to agcAvg(); "integrator control" = accumulated error

// variables used by getSample() and agcAvg()
static int16_t  micIn = 0;           // Current sample starts with negative values and large values, which is why it's 16 bit signed
static double   sampleMax = 0.0;     // Max sample over a few seconds. Needed for AGC controller.
static double   micLev = 0.0;        // Used to convert returned value to have '0' as minimum. A leveller
static float    expAdjF = 0.0f;      // Used for exponential filter.
static float    sampleReal = 0.0f;	  // "sampleRaw" as float, to provide bits that are lost otherwise (before amplification by sampleGain or inputLevel). Needed for AGC.
static int16_t  sampleRaw = 0;       // Current sample. Must only be updated ONCE!!! (amplified mic value by sampleGain and inputLevel)
static int16_t  rawSampleAgc = 0;    // not smoothed AGC sample

// 
// AGC presets
//  Note: in C++, "const" implies "static" - no need to explicitly declare everything as "static const"
// 
#define AGC_NUM_PRESETS 3 // AGC presets:          normal,   vivid,    lazy
const double agcSampleDecay[AGC_NUM_PRESETS]  = { 0.9994f, 0.9985f, 0.9997f}; // decay factor for sampleMax, in case the current sample is below sampleMax
const float agcZoneLow[AGC_NUM_PRESETS]       = {      32,      28,      36}; // low volume emergency zone
const float agcZoneHigh[AGC_NUM_PRESETS]      = {     240,     240,     248}; // high volume emergency zone
const float agcZoneStop[AGC_NUM_PRESETS]      = {     336,     448,     304}; // disable AGC integrator if we get above this level
const float agcTarget0[AGC_NUM_PRESETS]       = {     112,     144,     164}; // first AGC setPoint -> between 40% and 65%
const float agcTarget0Up[AGC_NUM_PRESETS]     = {      88,      64,     116}; // setpoint switching value (a poor man's bang-bang)
const float agcTarget1[AGC_NUM_PRESETS]       = {     220,     224,     216}; // second AGC setPoint -> around 85%
const double agcFollowFast[AGC_NUM_PRESETS]   = { 1/192.f, 1/128.f, 1/256.f}; // quickly follow setpoint - ~0.15 sec
const double agcFollowSlow[AGC_NUM_PRESETS]   = {1/6144.f,1/4096.f,1/8192.f}; // slowly follow setpoint  - ~2-15 secs
const double agcControlKp[AGC_NUM_PRESETS]    = {    0.6f,    1.5f,   0.65f}; // AGC - PI control, proportional gain parameter
const double agcControlKi[AGC_NUM_PRESETS]    = {    1.7f,   1.85f,    1.2f}; // AGC - PI control, integral gain parameter
const float agcSampleSmooth[AGC_NUM_PRESETS]  = {  1/12.f,   1/6.f,  1/16.f}; // smoothing factor for sampleAgc (use rawSampleAgc if you want the non-smoothed value)
// AGC presets end

#ifndef SR_SQUELCH
  uint8_t soundSquelch = 10;                  // squelch value for volume reactive routines (config value)
#else
  uint8_t soundSquelch = SR_SQUELCH;          // squelch value for volume reactive routines (config value)
#endif

/*
* A "PI controller" multiplier to automatically adjust sound sensitivity.
* 
* A few tricks are implemented so that sampleAgc does't only utilize 0% and 100%:
* 0. don't amplify anything below squelch (but keep previous gain)
* 1. gain input = maximum signal observed in the last 5-10 seconds
* 2. we use two setpoints, one at ~60%, and one at ~80% of the maximum signal
* 3. the amplification depends on signal level:
*    a) normal zone - very slow adjustment
*    b) emergency zone (<10% or >90%) - very fast adjustment
*/
void agcAvg()
{
  const int AGC_preset = (soundAgc > 0)? (soundAgc-1): 0; // make sure the _compiler_ knows this value will not change while we are inside the function

  float lastMultAgc = multAgc;      // last multiplier used
  float multAgcTemp = multAgc;      // new multiplier
  float tmpAgc = sampleReal * multAgc;        // what-if amplified signal

  float control_error;                        // "control error" input for PI control

  if (last_soundAgc != soundAgc)
    control_integrated = 0.0;                // new preset - reset integrator

  if((fabsf(sampleReal) < 2.0f) || (sampleMax < 1.0)) {
    // MIC signal is "squelched" - deliver silence
    tmpAgc = 0;
    // we need to "spin down" the intgrated error buffer
    if (fabs(control_integrated) < 0.01)  control_integrated  = 0.0;
    else                                  control_integrated *= 0.91;
  } else {
    // compute new setpoint
    if (tmpAgc <= agcTarget0Up[AGC_preset])
      multAgcTemp = agcTarget0[AGC_preset] / sampleMax;   // Make the multiplier so that sampleMax * multiplier = first setpoint
    else
      multAgcTemp = agcTarget1[AGC_preset] / sampleMax;   // Make the multiplier so that sampleMax * multiplier = second setpoint
  }
  // limit amplification
  if (multAgcTemp > 32.0f)      multAgcTemp = 32.0f;
  if (multAgcTemp < 1.0f/64.0f) multAgcTemp = 1.0f/64.0f;

  // compute error terms
  control_error = multAgcTemp - lastMultAgc;
  
  if (((multAgcTemp > 0.085f) && (multAgcTemp < 6.5f))    //integrator anti-windup by clamping
      && (multAgc*sampleMax < agcZoneStop[AGC_preset]))   //integrator ceiling (>140% of max)
    control_integrated += control_error * 0.002 * 0.25;   // 2ms = integration time; 0.25 for damping
  else
    control_integrated *= 0.9;                            // spin down that beasty integrator

  // apply PI Control 
  tmpAgc = sampleReal * lastMultAgc;                      // check "zone" of the signal using previous gain
  if ((tmpAgc > agcZoneHigh[AGC_preset]) || (tmpAgc < soundSquelch + agcZoneLow[AGC_preset])) {  // upper/lower energy zone
    multAgcTemp = lastMultAgc + agcFollowFast[AGC_preset] * agcControlKp[AGC_preset] * control_error;
    multAgcTemp += agcFollowFast[AGC_preset] * agcControlKi[AGC_preset] * control_integrated;
  } else {                                                                         // "normal zone"
    multAgcTemp = lastMultAgc + agcFollowSlow[AGC_preset] * agcControlKp[AGC_preset] * control_error;
    multAgcTemp += agcFollowSlow[AGC_preset] * agcControlKi[AGC_preset] * control_integrated;
  }

  // limit amplification again - PI controller sometimes "overshoots"
  //multAgcTemp = constrain(multAgcTemp, 0.015625f, 32.0f); // 1/64 < multAgcTemp < 32
  if (multAgcTemp > 32.0f)      multAgcTemp = 32.0f;
  if (multAgcTemp < 1.0f/64.0f) multAgcTemp = 1.0f/64.0f;

  // NOW finally amplify the signal
  tmpAgc = sampleReal * multAgcTemp;                  // apply gain to signal
  if (fabsf(sampleReal) < 2.0f) tmpAgc = 0.0f;        // apply squelch threshold
  //tmpAgc = constrain(tmpAgc, 0, 255);
  if (tmpAgc > 255) tmpAgc = 255.0f;                  // limit to 8bit
  if (tmpAgc < 1)   tmpAgc = 0.0f;                    // just to be sure

  // update global vars ONCE - multAgc, sampleAGC, rawSampleAgc
  multAgc = multAgcTemp;
  rawSampleAgc = 0.8f * tmpAgc + 0.2f * (float)rawSampleAgc;
  // update smoothed AGC sample
  if (fabsf(tmpAgc) < 1.0f) 
    sampleAgc =  0.5f * tmpAgc + 0.5f * sampleAgc;    // fast path to zero
  else
    sampleAgc += agcSampleSmooth[AGC_preset] * (tmpAgc - sampleAgc); // smooth path

  sampleAgc = fabsf(sampleAgc);                                      // // make sure we have a positive value
  last_soundAgc = soundAgc;
} // agcAvg()

// post-processing and filtering of MIC sample (micDataReal) from FFTcode()
void processSample(int16_t sample)
{
  float    sampleAdj;           // Gain adjusted sample value
  float    tmpSample;           // An interim sample variable used for calculations.
  const float weighting = 0.2f; // Exponential filter weighting. Will be adjustable in a future release.
  const int   AGC_preset = (soundAgc > 0)? (soundAgc-1): 0; // make sure the _compiler_ knows this value will not change while we are inside the function

  micIn = sample;

  micLev += (micDataReal-micLev) / 12288.0f;
  if(micIn < micLev) micLev = ((micLev * 31.0f) + micDataReal) / 32.0f; // align MicLev to lowest input signal

  micIn -= micLev;                                  // Let's center it to 0 now
  // Using an exponential filter to smooth out the signal. We'll add controls for this in a future release.
  float micInNoDC = fabsf(micDataReal - micLev);
  expAdjF = (weighting * micInNoDC + (1.0f-weighting) * expAdjF);
  expAdjF = fabsf(expAdjF);                         // Now (!) take the absolute value

  expAdjF = (expAdjF <= soundSquelch) ? 0: expAdjF; // simple noise gate
  if ((soundSquelch == 0) && (expAdjF < 0.25f)) expAdjF = 0; // do something meaningfull when "squelch = 0"

  tmpSample = expAdjF;
  micIn = abs(micIn);                               // And get the absolute value of each sample

  sampleAdj = tmpSample * sampleGain / 40.0f * inputLevel/128.0f + tmpSample / 16.0f; // Adjust the gain. with inputLevel adjustment
  sampleReal = tmpSample;

  sampleAdj = fmax(fmin(sampleAdj, 255), 0);        // Question: why are we limiting the value to 8 bits ???
  sampleRaw = (int16_t)sampleAdj;                   // ONLY update sample ONCE!!!!

  // keep "peak" sample, but decay value if current sample is below peak
  if ((sampleMax < sampleReal) && (sampleReal > 0.5f)) {
    sampleMax = sampleMax + 0.5f * (sampleReal - sampleMax);  // new peak - with some filtering
    // another simple way to detect samplePeak - cannot detect beats, but reacts on peak volume
    if (((binNum < 12) || ((maxVol < 1))) && (millis() - timeOfPeak > 80) && (sampleAvg > 1)) {
      samplePeak    = true;
      timeOfPeak    = millis();
      udpSamplePeak = true;
    }
  } else {
    if ((multAgc*sampleMax > agcZoneStop[AGC_preset]) && (soundAgc > 0))
      sampleMax += 0.5f * (sampleReal - sampleMax);        // over AGC Zone - get back quickly
    else
      sampleMax *= agcSampleDecay[AGC_preset];             // signal to zero --> 5-8sec
  }
  if (sampleMax < 0.5f) sampleMax = 0.0f;

  sampleAvg = ((sampleAvg * 15.0f) + sampleAdj) / 16.0f;   // Smooth it out over the last 16 samples.
  sampleAvg = fabsf(sampleAvg);                            // make sure we have a positive value
} // getSample()



  
