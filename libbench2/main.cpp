#include <iostream>
#include <vector>
#include <cmath>
#include <portaudio.h>

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAMES_PER_BUFFER = 256;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vec3 {
    float x, y, z;
};

struct StockPoint {
    float time;
    float price;
};

std::vector<StockPoint> stockData;
std::vector<Vec3> positions;

unsigned int playbackPos = 0;
float phase = 0.0f;

bool playbackFinished = false;

// 1. 가상 데이터 생성
void generateVirtualStockData(int N) {
    stockData.clear();
    positions.clear();
    float radius = 1.0f;

    for (int i = 0; i < N; ++i) {
        float price = 10 + 90 * (0.5f + 0.5f * sinf(i * 0.15f));
        stockData.push_back({ static_cast<float>(i), price });

        // 원형 대신 단순 직선 좌우 배치 (사용자 앞쪽 평면에 놓임)
        float x = -1.0f + 2.0f * i / (N - 1); // -1부터 +1까지 균일 분포
        float y = (price - 10) / 90.0f * 2.0f - 1.0f; // -1~1 정규화 높이
        float z = 0.0f;
        positions.push_back({ x, y, z });
    }
}

// 2. 가격 → 주파수 변환
float priceToFrequency(float price) {
    float minPrice = 10.0f, maxPrice = 100.0f;
    float minFreq = 200.0f, maxFreq = 1000.0f;

    if (price < minPrice) price = minPrice;
    if (price > maxPrice) price = maxPrice;

    return minFreq + (price - minPrice) / (maxPrice - minPrice) * (maxFreq - minFreq);
}

// 3. 좌우 패닝 (-1: 왼쪽, +1: 오른쪽)
float calcPanX(const Vec3& pos) {
    if (pos.x < -1.0f) return -1.0f;
    if (pos.x > 1.0f) return 1.0f;
    return pos.x;
}

// 4. 상하 볼륨 (0~1)
float calcVolY(const Vec3& pos) {
    float vol = (pos.y + 1.0f) / 2.0f;
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    return vol;
}

// 5. PortAudio 콜백 - 한 번만 재생하고 종료
unsigned int sampleCounter = 0;
const unsigned int samplesPerStep = SAMPLE_RATE / 2; // 0.5초마다 다음 점으로 이동

static int paCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    float* out = (float*)outputBuffer;
    int N = static_cast<int>(stockData.size());

    if (playbackFinished) {
        for (unsigned int i = 0; i < framesPerBuffer; ++i) {
            out[i * 2] = 0.0f;
            out[i * 2 + 1] = 0.0f;
        }
        return paComplete;
    }

    for (unsigned int i = 0; i < framesPerBuffer; ++i) {
        if (playbackPos >= N) {
            playbackFinished = true;
            out[i * 2] = 0.0f;
            out[i * 2 + 1] = 0.0f;
            continue;
        }

        float freq = priceToFrequency(stockData[playbackPos].price);
        float sample = sinf(phase);

        float pan = calcPanX(positions[playbackPos]);
        float vol = calcVolY(positions[playbackPos]);

        float left = sample * (1.0f - pan) * 0.5f * vol;
        float right = sample * (1.0f + pan) * 0.5f * vol;

        out[i * 2] = left;
        out[i * 2 + 1] = right;

        phase += 2.0f * M_PI * freq / SAMPLE_RATE;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;

        // 재생 속도 조절: 샘플 개수 기준으로 위치 증가
        sampleCounter++;
        if (sampleCounter >= samplesPerStep) {
            sampleCounter = 0;
            playbackPos++;
        }
    }
    return paContinue;
}

int main() {
    generateVirtualStockData(100);

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio init error: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream,
        0, 2, paFloat32, SAMPLE_RATE,
        FRAMES_PER_BUFFER, paCallback, nullptr);
    if (err != paNoError) {
        std::cerr << "PortAudio open stream error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return -1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio start stream error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return -1;
    }

    std::cout << "Playing graph sound from left to right, price mapped to height." << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    return 0;
}
