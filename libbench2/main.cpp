#include <iostream>
#include <vector>
#include <cmath>
#include <portaudio.h>

#define M_PI 3.14159265358979323846

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAMES_PER_BUFFER = 256;

// ������ 3D ����
struct Vec3 {
    float x, y, z;
};

// �ֽ� ������ ����Ʈ (�ӽ�)
struct StockPoint {
    float time;   // �ε����� �ð� (�ӽ�)
    float price;  // ���� ��
};

// ���� ������
std::vector<StockPoint> stockData;
std::vector<Vec3> positions; // 3D ��ġ

// ���� ��� ��ġ (�ε���)
unsigned int playbackPos = 0;
float phase = 0.0f;

// ���� �� ���ļ� ���� �Լ� (��: 200Hz ~ 1000Hz)
float priceToFreq(float price) {
    float minPrice = 10.0f;
    float maxPrice = 100.0f;
    float minFreq = 200.0f;
    float maxFreq = 1000.0f;
    price = std::min(std::max(price, minPrice), maxPrice);
    return minFreq + (price - minPrice) / (maxPrice - minPrice) * (maxFreq - minFreq);
}

// 3D ��ǥ�� ������ �¿� �д� ������ ��ȯ (HRTF ���)
float calcPan(const Vec3& pos) {
    // x�� ���� -1 (����) ~ +1 (������)
    return std::max(-1.0f, std::min(1.0f, pos.x));
}

// �ֽ� ������ �ӽ� ����
void generateDummyStockData(int N) {
    stockData.clear();
    positions.clear();

    for (int i = 0; i < N; ++i) {
        float price = 10 + 90 * (0.5f + 0.5f * sinf(i * 0.1f)); // ���� ���� ����
        stockData.push_back({ (float)i, price });
    }

    // �������� 3D ��ġ ��� (������ 1.0)
    float radius = 1.0f;
    for (int i = 0; i < N; ++i) {
        float angle = (float)i / N * 2.0f * M_PI;
        float x = radius * cosf(angle);
        float y = (stockData[i].price - 10) / 90.0f * 2.0f - 1.0f; // ���� -1~1 ����ȭ
        float z = radius * sinf(angle);
        positions.push_back({ x, y, z });
    }
}

// PortAudio �ݹ� �Լ�
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
        // ���� ����Ʈ �ε���
        int idx = playbackPos % N;

        // ���� ����Ʈ ���ݿ� ���� ���ļ�
        float freq = priceToFreq(stockData[idx].price);

        // ������ ����
        float sample = sinf(phase);
        phase += 2.0f * M_PI * freq / SAMPLE_RATE;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;

        // ������ �д� ��� (�¿� ����)
        float pan = calcPan(positions[idx]);
        float left = sample * (1.0f - pan) * 0.5f;
        float right = sample * (1.0f + pan) * 0.5f;

        // ��� ���ۿ� �ۼ� (���׷���)
        out[i * 2] = left;
        out[i * 2 + 1] = right;

        // ���� ���������� �ε��� �̵� (�ӵ� ���� ����)
        if (i % 20 == 0) // �ӵ� ���� (õõ�� �����̵���)
            playbackPos = (playbackPos + 1) % N;
    }

    return paContinue;
}

int main() {
    // ������ ����
    generateDummyStockData(100);

    // PortAudio �ʱ�ȭ
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio init error: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    // ��Ʈ�� ����
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream,
        0, // �Է� ä�� 0
        2, // ��� ä�� 2 (���׷���)
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

    // ��Ʈ�� ����
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio start stream error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return -1;
    }

    std::cout << "3D Audio Stock Graph Demo. Press Enter to stop..." << std::endl;
    std::cin.get();

    // ��Ʈ�� ����
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    return 0;
}
