#include "MicroPhone.h"

#include "arduinoFFT.h"
#include <cstdint>
#include <PDM.h>

// buffer to read samples into, each sample is 16-bits
int16_t _sampleBuffer[256];
volatile int samplesRead;

// 20 - 200hz Single Pole Bandpass IIR Filter
float bassFilter(float sample) {
    static float xv[3] = {0,0,0};
    static float yv[3] = {0,0,0};
    xv[0] = xv[1];
    xv[1] = xv[2]; 
    xv[2] = sample / 9.1f;
    yv[0] = yv[1];
    yv[1] = yv[2]; 
    yv[2] = (xv[2] - xv[0])
        + (-0.7960060012f * yv[0]) + (1.7903124146f * yv[1]);
    return yv[2];
}

// 10hz Single Pole Lowpass IIR Filter
float envelopeFilter(float sample) { //10hz low pass
    static float xv[2] = {0,0};
    static float yv[2] = {0,0};
    xv[0] = xv[1]; 
    xv[1] = sample / 160.f;
    yv[0] = yv[1]; 
    yv[1] = (xv[0] + xv[1]) + (0.9875119299f * yv[0]);
    return yv[1];
}

// 1.7 - 3.0hz Single Pole Bandpass IIR Filter
float beatFilter(float sample) {
    static float xv[3] = {0,0,0};
    static float yv[3] = {0,0,0};
    xv[0] = xv[1];
    xv[1] = xv[2]; 
    xv[2] = sample / 7.015f;
    
    yv[0] = yv[1];
    yv[1] = yv[2]; 
    yv[2] = (xv[2] - xv[0])
        + (-0.7169861741f * yv[0]) + (1.4453653501f * yv[1]);
    return yv[2];
}

/**
 * \brief Return true if a beat is detected
 */
bool beat_judge(float val){
    static float history[10];
    int i = 0;
    float avg =0;

    //compute average of last 9 samples (hopefully)
    for(i = 9; i >= 0; i--){
        avg += history[i];
    }
    avg = avg/9;


    //write history (heh, see what I did there? no? nevermind. Just pushing newest value on FIFO)
    for(i = 0; i< 8; i++){
        history[i] = history[i+1];
    }
    history[9] = val;
    //basically we got fast rise in low freq volume
    //"magic" (adapt this garbage to something that works better, if possible)
    return (avg * 145) < (val - 45);
}

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


void init_microphone(const uint32_t sampleRate)
{
    PDM.onReceive(on_PDM_data);

  // optionally set the gain, defaults to 20
  // PDM.setGain(30);

  // initialize PDM with:
  // - one channel (mono mode)
  // - a sample rate
  if (!PDM.begin(1, sampleRate))
  {
    // block program execution
    while (1)
    {
        // slow blink if failed
        digitalToggle(LED_BUILTIN);
        delay(1000);
    }
  }
}

uint32_t lastUpdateMicros;
float get_beat_probability()
{
    static const uint32_t updatePeriod = 4000;//1.0/25.0 * 1e6; // 25Hz in micro seconds
    if(!samplesRead)
        return 0.0;

    bool isBeatDetected = false;

    const uint32_t newUpdateMicros = micros();
    // average: 64 uS by sample
    const uint32_t measurmentDurationMicros = lastMeasurmentDurationMicros / (float)samplesRead; 

    for (int i = 0; i < samplesRead; i++) {
        const float sample = _sampleBuffer[i] / 2048.0 * 512.0;

        // Filter only bass component
        const float value = abs(bassFilter(sample));

        static float passHaut = 0;
        // Take signal amplitude and filter (basically get the envelope of the low freq signals)
        const float envelope = passHaut * 0.99 + (1.0 - 0.99) * envelopeFilter(value);
        passHaut = envelope;
    
        // Every 25hz filter the envelope
        const uint32_t projectedMeasurmentTime = lastMeasurmentMicros + i * measurmentDurationMicros;
        if(projectedMeasurmentTime - lastUpdateMicros >= updatePeriod) {
            lastUpdateMicros = projectedMeasurmentTime;
            // Filter out repeating bass sounds 100 - 180bpm
            const float beat = beatFilter(envelope);
            
            Serial.print(envelope);
            Serial.print(",");
            Serial.println(beat);
            isBeatDetected = isBeatDetected | beat_judge(beat);
        }
    }

    samplesRead = 0;
    return isBeatDetected;
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