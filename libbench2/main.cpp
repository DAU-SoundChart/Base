#include <iostream>
#include <vector>
#include <cmath>
#include <portaudio.h>

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAMES_PER_BUFFER = 256;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 시야각은 라디안 단위로 관리 (나중에 degree로 변환 가능)
float horizontalFOV = 160.0f * M_PI / 180.0f; // 좌우 시야, ±20도
float verticalFOV = 60.0f * M_PI / 180.0f;   // 상하 시야, ±30도

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

        // 각도 범위를 시야각 내로 제한 (좌우 시야 범위)
        float angle = -horizontalFOV / 2.0f + (float)i / (N - 1) * horizontalFOV;
        float x = radius * sinf(angle);   // 좌우 방향 (sin)
        float z = radius * cosf(angle);   // 앞 방향 (cos)
        float y = (price - 10) / 90.0f * 2.0f - 1.0f; // 높이 -1~1

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
    // pos.x는 이미 -sin(시야각) 범위 내
    // pos.x를 [-1, 1]로 정규화(이미 시야 내 배치라 그대로 사용)
    float pan = pos.x;
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    return pan;
}

// 4. 상하 볼륨 (0~1)
float calcVolY(const Vec3& pos) {
    // pos.y는 -1~1 사이로 높이 정규화 되어 있음
    // verticalFOV 각도와 연동하는 감쇠는 선택 사항
    float vol = (pos.y + 1.0f) / 2.0f; // 0~1 볼륨으로 변환
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    return vol;
}

// 5. PortAudio 콜백 - 한 번만 재생하고 종료
unsigned int sampleCounter = 0;
const unsigned int samplesPerStep = SAMPLE_RATE / 4; // 0.25초마다 다음 점으로 이동

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

// 그래프 그려봄
void printStockDataAndPositions() {
    std::cout << "Index\tTime\tPrice\tX\tY\tZ\n";
    for (size_t i = 0; i < stockData.size(); ++i) {
        std::cout
            << i << "\t"
            << stockData[i].time << "\t"
            << stockData[i].price << "\t"
            << positions[i].x << "\t"
            << positions[i].y << "\t"
            << positions[i].z << "\n";
    }
}

int main() {
    generateVirtualStockData(30);

    printStockDataAndPositions();

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
