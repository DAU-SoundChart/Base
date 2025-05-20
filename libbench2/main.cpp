#include <iostream>
#include <vector>
#include <cmath>
#include <portaudio.h>

#define M_PI 3.14159265358979323846

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAMES_PER_BUFFER = 256;

// 간단한 3D 벡터
struct Vec3 {
    float x, y, z;
};

// 주식 데이터 포인트 (임시)
struct StockPoint {
    float time;   // 인덱스나 시간 (임시)
    float price;  // 가격 값
};

// 전역 데이터
std::vector<StockPoint> stockData;
std::vector<Vec3> positions; // 3D 위치

// 사운드 재생 위치 (인덱스)
unsigned int playbackPos = 0;
float phase = 0.0f;

// 가격 → 주파수 매핑 함수 (예: 200Hz ~ 1000Hz)
float priceToFreq(float price) {
    float minPrice = 10.0f;
    float maxPrice = 100.0f;
    float minFreq = 200.0f;
    float maxFreq = 1000.0f;
    price = std::min(std::max(price, minPrice), maxPrice);
    return minFreq + (price - minPrice) / (maxPrice - minPrice) * (maxFreq - minFreq);
}

// 3D 좌표를 간단히 좌우 패닝 비율로 변환 (HRTF 대신)
float calcPan(const Vec3& pos) {
    // x축 기준 -1 (왼쪽) ~ +1 (오른쪽)
    return std::max(-1.0f, std::min(1.0f, pos.x));
}

// 주식 데이터 임시 생성
void generateDummyStockData(int N) {
    stockData.clear();
    positions.clear();

    for (int i = 0; i < N; ++i) {
        float price = 10 + 90 * (0.5f + 0.5f * sinf(i * 0.1f)); // 임의 변동 가격
        stockData.push_back({ (float)i, price });
    }

    // 원형으로 3D 위치 계산 (반지름 1.0)
    float radius = 1.0f;
    for (int i = 0; i < N; ++i) {
        float angle = (float)i / N * 2.0f * M_PI;
        float x = radius * cosf(angle);
        float y = (stockData[i].price - 10) / 90.0f * 2.0f - 1.0f; // 높이 -1~1 정규화
        float z = radius * sinf(angle);
        positions.push_back({ x, y, z });
    }
}

// PortAudio 콜백 함수
static int paCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    float* out = (float*)outputBuffer;

    int N = stockData.size();
    if (N == 0) return paContinue;

    for (unsigned int i = 0; i < framesPerBuffer; ++i) {
        // 현재 포인트 인덱스
        int idx = playbackPos % N;

        // 현재 포인트 가격에 따른 주파수
        float freq = priceToFreq(stockData[idx].price);

        // 사인파 생성
        float sample = sinf(phase);
        phase += 2.0f * M_PI * freq / SAMPLE_RATE;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;

        // 간단한 패닝 계산 (좌우 볼륨)
        float pan = calcPan(positions[idx]);
        float left = sample * (1.0f - pan) * 0.5f;
        float right = sample * (1.0f + pan) * 0.5f;

        // 출력 버퍼에 작성 (스테레오)
        out[i * 2] = left;
        out[i * 2 + 1] = right;

        // 다음 프레임으로 인덱스 이동 (속도 조절 가능)
        if (i % 20 == 0) // 속도 조절 (천천히 움직이도록)
            playbackPos = (playbackPos + 1) % N;
    }

    return paContinue;
}

int main() {
    // 데이터 생성
    generateDummyStockData(100);

    // PortAudio 초기화
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio init error: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    // 스트림 생성
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream,
        0, // 입력 채널 0
        2, // 출력 채널 2 (스테레오)
        paFloat32,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paCallback,
        nullptr);
    if (err != paNoError) {
        std::cerr << "PortAudio open stream error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return -1;
    }

    // 스트림 시작
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio start stream error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return -1;
    }

    std::cout << "3D Audio Stock Graph Demo. Press Enter to stop..." << std::endl;
    std::cin.get();

    // 스트림 종료
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    return 0;
}
