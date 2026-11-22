# Oddlands (오드랜즈)

2026-2 컴퓨터 그래픽스 최종 프로젝트 · 홍범기, 김민우

## 처음 받았을 때

1. 저장소를 아무 폴더에나 받는다 (경로에 한글이 없으면 더 안전하다).
2. `Oddlands.sln`을 연다.
   - **Visual Studio 2022**: 위쪽 구성이 **Debug | x64**인지 확인하고 F5로 실행한다.
   - **Rider**: 오른쪽 위 구성을 **Debug | x64**로 두고 실행 버튼을 누른다.
     Rider는 빌드에 MSVC 빌드 도구를 쓰므로, Visual Studio 2022나
     'Build Tools for Visual Studio 2022'의 **C++를 사용한 데스크톱 개발**이 설치되어 있어야 한다.

GLEW·GLFW·GLM을 따로 설치하거나 Windows Kits 폴더에 복사할 필요가 없다.
필요한 헤더·lib·dll이 전부 `External` 폴더에 들어 있고, 프로젝트 설정이 이 폴더를 상대 경로로 가리킨다.

## 폴더 구조

```
Oddlands/
├─ Oddlands.sln
├─ External/                 ← 라이브러리 (수정하지 않음)
│  ├─ include/gl/            glew.h, glfw3.h, glm/
│  ├─ lib/x64/               glew32.lib, glfw3.lib
│  ├─ bin/x64/               glew32.dll (빌드 후 실행 폴더로 자동 복사)
│  └─ licenses/
├─ Oddlands/
│  ├─ Oddlands.vcxproj
│  ├─ src/main.cpp           코드 전부 (실습과 같은 구조)
│  └─ shaders/               vertex.glsl / fragment.glsl (실습과 같음, 빌드 후 실행 폴더로 자동 복사)
├─ bin/Debug, bin/Release    ← 실행 파일 (git에 안 올라감)
└─ obj/                      ← 중간 파일 (git에 안 올라감)
```

## 함께 작업할 때 규칙

- **include는 강의와 같은 형태로 쓴다.**
  `#include <gl/glew.h>`, `#include <gl/glfw3.h>`, `#include <gl/glm/glm.hpp>`
  (glew.h는 항상 glfw3.h보다 먼저)
- **프로젝트 속성에 절대 경로(`C:\...`)를 넣지 않는다.** 경로가 필요하면 `$(ProjectDir)` 기준 상대 경로를 쓴다.
- **코드는 `main.cpp` 하나에 실습과 같은 구조로 쓴다.** 위쪽 뼈대(`ShaderUtil`, `Mesh3D`, `DrawBox`, `Window`)는 실습 코드 그대로이고, 아래 "게임" 부분에 기능별 `namespace`(예: `Player`, `Camera`)를 추가한다. 실습과 다르게 한 곳에는 `[추가]` 주석을 단다.
- 나중에 파일을 나눠야 하면 IDE의 프로젝트 창에서 추가한다 (VS: 추가 → 새 항목, Rider: 프로젝트 우클릭 → Add). 그래야 `Oddlands.vcxproj`에 등록된다.
- 셰이더는 `Oddlands/shaders/`에 두고 코드에서는 `"shaders/파일이름"`으로 읽는다. 프로그램이 시작할 때 작업 폴더를 실행 파일 위치로 바꾸므로, 어떤 IDE에서 실행해도 같은 경로로 읽힌다.
- 플랫폼은 x64만 쓴다 (lib가 64비트용).

## 빌드 환경 테스트

Claude Code를 이 폴더에서 열고 "TEST_WITH_CLAUDE_CODE.md 대로 테스트하고 결과를 보고해줘"라고 요청하면 된다.

## 제출용 실행 파일

Release|x64로 빌드하면 `bin/Release/` 폴더에 `Oddlands.exe`, `glew32.dll`, `shaders/`가 모인다.
이 폴더를 통째로 압축하면 다른 PC에서도 바로 실행된다.

## 조작 (현재)

- W A S D: 이동 (월드 기준, W가 화면 안쪽)
- Space: 점프 (최고 높이 약 2.5. 나무 상자·바위와 적은 넘을 수 있고, 기둥·나무는 돌아가야 한다)
- 공격: 자동 (바라보는 방향으로 0.5초마다 발사)
- 1 / 2 / 3: 레벨업 때 업그레이드 선택 (UI가 생기기 전까지 선택지는 콘솔 창에 나온다)
- ESC: 종료

HUD가 생기기 전까지 체력·레벨·경험치·시간·처치 수는 **창 제목**에 표시된다.
