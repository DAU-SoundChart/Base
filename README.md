# SoundChart

## Version
- Visual Studio 2022

## 실행 방법
1. build/fftw.sln 솔루션 파일 Visual Studio로 실행
2. 프로젝트 패널에서 bench 프로젝트 우클릭 후 속성 클릭
3. 구성 속성 - C/C++ - 일반 - 추가 포함 디렉터리 [편집]
- ..\portaudio-19.7.0\include
- ..
- ..\build
4. 구성 속성 - 링커 - 일반 - 추가 라이브러리 디렉터리 [편집]
- ..\portaudio-19.7.0\build\Debug
5. 구성 속성 - 링커 - 입력 - 추가 종속성 [편집]
- portaudio_x64.lib (추가)
6. bench 프로젝트 우클릭 후 '시작 프로그램으로 설정'
7. Ctrl + Shift + F5 : 프로젝트 빌드
8. 실행

## Ref
- [CMake - 4.0.2](https://cmake.org/download/)
- [FFTW Download - 3.3.10](https://www.fftw.org/download.html)
- [PortAudio - 19.7.0](https://github.com/PortAudio/portaudio/releases)
