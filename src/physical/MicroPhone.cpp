#include "MicroPhone.h"

#include "fft.h"
#include <cstdint>
#include <PDM.h>

namespace sound {

// audio source parameters and constant
//constexpr double SAMPLE_RATE = 22050;        // Base sample rate in Hz - 22Khz is a standard rate. Physical sample time -> 23ms
//constexpr double SAMPLE_RATE = 16000;        // 16kHz - use if FFTtask takes more than 20ms. Physical sample time -> 32ms
//constexpr double SAMPLE_RATE = 20480;        // Base sample rate in Hz - 20Khz is experimental.    Physical sample time -> 25ms
//constexpr double SAMPLE_RATE = 10240;        // Base sample rate in Hz - previous default.         Physical sample time -> 50ms

// buffer to read samples into, each sample is 16-bits
constexpr size_t sampleSize = samplesFFT;
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
    PDM.read(_sampleBuffer, bytesAvailable);

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

    PDM.setBufferSize(sampleSize);
    PDM.onReceive(on_PDM_data);

    // optionally set the gain, defaults to 20
    PDM.setGain(30);

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

bool processFFT()
{
    if(samplesRead <= 0)
    {
        return false;
    }

    // get data
    uint32_t userloopDelay = LOOP_UPDATE_PERIOD;
    for(uint i = 0; i < samplesRead; i++)
    {
        float sample = _sampleBuffer[i];

        processSample(sample);
        agcAvg();

        vReal[i] = sample;
    }

    samplesRead = 0;
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

}