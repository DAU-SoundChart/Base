#include <iostream>
#include <cmath>
#include <portaudio.h>

using namespace std;

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAMES_PER_BUFFER = 256;

static int paCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    float* out = (float*)outputBuffer;
    static float phase = 0.0f;
    float frequency = 440.0f; // A4 tone
    float pan = 0.0f; // -1: left, 0 center, +1 right

    for (unsigned int i = 0; i < framesPerBuffer; i++) {
        float sample = sinf(phase);
        phase += 2.0f * 3.14159265f * frequency / SAMPLE_RATE;
        if (phase > 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;

        float left = sample * (1.0f - pan) * 0.5f;
        float right = sample * (1.0f + pan) * 0.5f;

        out[i * 2] = left;
        out[i * 2 + 1] = right;
    }
    return paContinue;
}

int main() {
    Pa_Initialize();
    PaStream* stream;
    Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, paCallback, nullptr);
    Pa_StartStream(stream);

    cout << "Playing sine wave. Press Enter to stop..." << endl;
    getchar();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}
