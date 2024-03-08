#include "MicroPhone.h"

#include "fft.h"
#include <algorithm>
#include <cstdint>
#include <PDM.h>

#include "../ext/random8.h"
#include "../ext/noise.h"

namespace sound {

// buffer to read samples into, each sample is 16-bits
constexpr size_t sampleSize = 512;//samplesFFT;
int16_t _sampleBuffer[sampleSize];
volatile int samplesRead;

uint32_t lastMeasurmentMicros;
uint32_t lastMeasurmentDurationMicros;
void on_PDM_data()
{
    const uint32_t newTime = micros();
    lastMeasurmentDurationMicros = newTime - lastMeasurmentMicros;
    lastMeasurmentMicros = newTime;
    // query the number of bytes available
    const int bytesAvailable = PDM.available();

    // read into the sample buffer
    PDM.read((char *)&_sampleBuffer[0], bytesAvailable);

    // number of samples read
    samplesRead = bytesAvailable / 2;
}


bool isStarted = false;
void enable_microphone()
{
    if (isStarted)
    {
        return;
    }

    PDM.setBufferSize(512);
    PDM.onReceive(on_PDM_data);

    // initialize PDM with:
    // - one channel (mono mode)
    // - a sample rate
    if (!PDM.begin(1, SAMPLE_RATE))
    {
        // block program execution
        while (1)
        {
            // slow blink if failed
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(1000);
        }
    }

    // optionally set the gain, defaults to 20
    PDM.setGain(30);

    micDataReal = 0.0f;
    volumeRaw = 0; volumeSmth = 0;
    sampleAgc = 0; sampleAvg = 0;
    sampleRaw = 0; rawSampleAgc = 0;
    my_magnitude = 0; FFT_Magnitude = 0; FFT_MajorPeak = 1;
    multAgc = 1;
    // reset FFT data
    memset(fftCalc, 0, sizeof(fftCalc)); 
    memset(fftAvg, 0, sizeof(fftAvg)); 
    memset(fftResult, 0, sizeof(fftResult)); 
    for(int i=0; i<NUM_GEQ_CHANNELS; i+=2) fftResult[i] = 16; // make a tiny pattern
    inputLevel = 128;                                    // reset level slider to default
    autoResetPeak();

    isStarted = true;
}

void disable_microphone()
{
    if (!isStarted)
    {
        return;
    }

    PDM.end();
    isStarted = false;
}

float get_sound_level_Db()
{
    static float lastValue = 0;

    if(!samplesRead)
        return lastValue;

    float sumOfAll = 0.0;
    for (int i = 0; i < samplesRead; i++)
    {
        sumOfAll += powf(_sampleBuffer[i] / (float)1024.0, 2.0);
    }
    const float average = sumOfAll / (float)samplesRead;

    lastValue = 20.0 * log10f(sqrtf(average));
    // convert to decibels
    return lastValue;
}

bool processFFT(const bool runFFT = true)
{
    if(samplesRead <= 0)
    {
        return false;
    }

    // get data
    uint32_t userloopDelay = LOOP_UPDATE_PERIOD;
    for(int i = 0; i < samplesRead; i++)
    {
        float sample = (_sampleBuffer[i] & 0xFFFF);

        processSample(sample);
        agcAvg();

        vReal[i] = sampleRaw;
        vImag[i] = 0;
    }
    for(int i = samplesRead; i < samplesFFT; i++)
    {
        vReal[i] = 0;
        vImag[i] = 0;
    }

    volumeSmth = (soundAgc) ? sampleAgc   : sampleAvg;
    volumeRaw  = (soundAgc) ? rawSampleAgc: sampleRaw;
    // update FFTMagnitude, taking into account AGC amplification
    my_magnitude = FFT_Magnitude; // / 16.0f, 8.0f, 4.0f done in effects
    if (soundAgc) my_magnitude *= multAgc;
    if (volumeSmth < 1 ) my_magnitude = 0.001f;  // noise gate closed - mute
    limitSampleDynamics();
    autoResetPeak();

    samplesRead = 0;
    if(runFFT)
        FFTcode();

    return true;
}

void vu_meter(const Color& vuColor, LedStrip& strip)
{
  const float decibels = get_sound_level_Db();
  // convert to 0 - 1
  const float vuLevel = (decibels + abs(silenceLevelDb)) / highLevelDb;

  // display the gradient
  strip.clear();
  animations::fill(vuColor, strip, vuLevel);
}

void fftDisplay(const uint8_t speed, const uint8_t scale, const palette_t& palette, const bool reset, LedStrip& strip, const uint8_t nbBands)
{
    const int NUM_BANDS = map(nbBands, 0, 255, 1, 16);
    const uint16_t cols = ceil(stripXCoordinates);
    const uint16_t rows = ceil(stripYCoordinates);

    static uint32_t lastCall = 0;
    static uint32_t call = 0;

    static uint16_t* previousBarHeight = strip._buffer16b;

    if(reset or call == 0)
    {
        lastCall = 0;
        call = 0;

        memset(strip._buffer16b, 0, sizeof(strip._buffer16b));
    }

    bool rippleTime = false;
    if (millis() - lastCall >= (256U - scale)) {
        lastCall = millis();
        rippleTime = true;
    }

    // process the sound input
    if (!processFFT())
    {
        return;
    }

    int fadeoutDelay = (256 - speed) / 64;
    if ((fadeoutDelay <= 1 ) || ((call % fadeoutDelay) == 0)) strip.fadeToBlackBy(speed);

    for (int x=0; x < cols; x++) {
        uint8_t  band       = map(x, 0, cols-1, 0, NUM_BANDS - 1);
        if (NUM_BANDS < 16) band = map(band, 0, NUM_BANDS - 1, 0, 15); // always use full range. comment out this line to get the previous behaviour.
        band = constrain(band, 0, 15);
        uint16_t barHeight  = map(fftResult[band], 0, 255, 0, rows); // do not subtract -1 from rows here
        if (barHeight > previousBarHeight[x]) previousBarHeight[x] = barHeight; //drive the peak up
        uint32_t ledColor = 0; // black
        for (int y=0; y < barHeight; y++) {
            uint8_t colorIndex = map(y, 0, rows-1, 0, 255);

            ledColor = get_color_from_palette(colorIndex, palette);
            strip.setPixelColorXY(x, rows - y, ledColor);
        }
        if (previousBarHeight[x] > 0)
        {
            strip.setPixelColorXY(x, rows - 1 - previousBarHeight[x], ledColor);
        }

        if (rippleTime && previousBarHeight[x]>0) previousBarHeight[x]--;    //delay/ripple effect
    }
}

//4 bytes
typedef struct Ripple {
  uint8_t state;
  uint8_t color;
  uint16_t pos;
} ripple;

void mode_ripplepeak(const uint8_t rippleNumber, const palette_t& palette, LedStrip& strip) {                // * Ripple peak. By Andrew Tuline.
                                                          // This currently has no controls.
    #define maxsteps 16                                     // Case statement wouldn't allow a variable.

    static uint32_t* ripplesBuffer = strip.get_buffer_ptr(0);
    static uint8_t fadeLevel = 255;

    uint16_t maxRipples = 128;
    Ripple* ripples = reinterpret_cast<Ripple*>(ripplesBuffer);

    strip.fadeToBlackBy(240);                                  // Lower frame rate means less effective fading than FastLED
    strip.fadeToBlackBy(240);

    COLOR cFond;
    COLOR c;

    processFFT();   // ignore return

    const uint8_t rippleCount = map(rippleNumber, 0, 255, 0, maxRipples);
    for (int i = 0; i < rippleCount; i++) {   // Limit the number of ripples.
        if (samplePeak) ripples[i].state = 255;

        switch (ripples[i].state) {
        case 254:     // Inactive mode
            break;

        case 255:                                           // Initialize ripple variables.
            ripples[i].pos = random16(LED_COUNT);
            if (FFT_MajorPeak > 1)                          // log10(0) is "forbidden" (throws exception)
            ripples[i].color = (int)(log10f(FFT_MajorPeak)*128);
            else ripples[i].color = 0;
            
            ripples[i].state = 0;
            break;

        case 0:
            cFond.color = strip.getPixelColor(i);
            
            c.color = get_color_from_palette(ripples[i].color, palette);
            strip.setPixelColor(ripples[i].pos, utils::color_blend(cFond, c, fadeLevel));
            ripples[i].state++;
            break;

        case maxsteps:                                      // At the end of the ripples. 254 is an inactive mode.
            ripples[i].state = 254;
            break;

        default:                                            // Middle of the ripples.
            cFond.color = strip.getPixelColor(i);
            c.color = get_color_from_palette(ripples[i].color, palette);
            strip.setPixelColor((ripples[i].pos + ripples[i].state + LED_COUNT) % LED_COUNT, utils::color_blend(cFond, c, fadeLevel/ripples[i].state*2));
            strip.setPixelColor((ripples[i].pos - ripples[i].state + LED_COUNT) % LED_COUNT, utils::color_blend(cFond, c, fadeLevel/ripples[i].state*2));
            ripples[i].state++;                               // Next step.
            break;
        } // switch step
    } // for i
} // mode_ripplepeak()


void mode_2DWaverly(const uint8_t speed, const uint8_t scale, const palette_t& palette, LedStrip& strip) {
    const uint16_t cols = ceil(stripXCoordinates);
    const uint16_t rows = ceil(stripYCoordinates);

    if (!processFFT(false))
        return;

    strip.fadeToBlackBy(speed);

    long t = millis() / 2;
    for (int i = 0; i < cols; i++) {
        uint16_t thisVal = (1 + scale/64) * noise8::inoise(i * 45 , t , t)/2;
        // use audio if available
        thisVal /= 32; // reduce intensity of inoise8()
        thisVal *= max(10.0, volumeSmth); // set a min size to always have the visual
        thisVal = constrain(thisVal, 0, 512);
        uint16_t thisMax = map(thisVal, 0, 512, 0, rows/2);
        for (int j = 0; j < thisMax; j++) {
            const auto color = get_color_from_palette((uint8_t)map(j, 0, thisMax, 250, 0), palette);
            strip.addPixelColorXY(i, j, color);
            strip.addPixelColorXY((cols - 1) - i, (rows - 1) - j, color);
        }
    }
    strip.blur(16);
} // mode_2DWaverly()

}