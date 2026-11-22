# Claude Code용 테스트 안내서

이 파일은 Claude Code가 이 저장소의 빌드 환경을 검증할 때 따라 하는 절차다.
사용자는 저장소 폴더에서 Claude Code를 열고 이렇게 요청하면 된다.

> TEST_WITH_CLAUDE_CODE.md 대로 테스트하고 결과를 보고해줘

---

## Claude Code에게

이 저장소는 Visual Studio 2022 형식의 C++ 프로젝트(OpenGL, GLEW, GLFW, GLM)다. 사용자는 IDE로 JetBrains Rider를 쓰고, 빌드는 PC에 설치된 MSVC 빌드 도구(MSBuild)로 한다.
목표는 **라이브러리를 따로 설치하지 않은 PC에서도, 저장소를 어느 경로에 두든 빌드·실행되는지** 확인하는 것이다.

지켜야 할 것:
- 소스 코드와 프로젝트 설정은 **고치지 않는다.** 테스트가 실패하면 원인과 고칠 방법을 보고만 한다. 사용자가 허락하면 그때 고친다.
- `External/` 폴더의 파일은 절대 수정하거나 지우지 않는다.
- 명령은 PowerShell 기준으로 실행한다.
- 각 단계의 결과(성공/실패, 핵심 출력)를 기록해 두었다가 마지막에 표로 보고한다.

### 1단계: MSBuild 찾기

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
$msbuild
```

- `-products *`는 Visual Studio 본체 대신 'Build Tools for Visual Studio'만 깔린 경우도 찾기 위한 옵션이다.
- `vswhere.exe`가 없거나 경로가 나오지 않으면 MSVC 빌드 도구('C++를 사용한 데스크톱 개발' 워크로드)가 없는 것이다. Rider 설정(File → Settings → Build, Execution, Deployment → Toolset and Build)에 MSBuild 경로가 잡혀 있는지 확인해 보고, 그래도 없으면 여기서 멈추고 보고한다.

### 2단계: Debug, Release 빌드

저장소 루트(`Oddlands.sln`이 있는 폴더)에서 실행한다.

```powershell
& $msbuild Oddlands.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
& $msbuild Oddlands.sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo
```

- 오류(error)가 0개면 성공이다.
- `LNK4098` 경고(기본 라이브러리 충돌)는 Debug 빌드에서 나올 수 있고, 무시해도 된다.
- `MSB8020`(플랫폼 도구 집합 v143 없음)이 나오면 설치된 VS 버전을 보고한다.

### 3단계: 실행 폴더 확인

두 구성 모두 아래 파일이 있어야 한다.

```powershell
foreach ($c in "Debug","Release") {
  "bin\$c\Oddlands.exe","bin\$c\glew32.dll","bin\$c\shaders\vertex.glsl","bin\$c\shaders\fragment.glsl" |
    ForEach-Object { "{0,-35} {1}" -f $_, (Test-Path $_) }
}
```

### 4단계: 실행 테스트 (5초)

창이 뜨는 프로그램이라 5초 뒤에 직접 종료한다. 5초 전에 스스로 꺼지면 실패(크래시)다.

```powershell
$p = Start-Process -FilePath "bin\Debug\Oddlands.exe" -WorkingDirectory "bin\Debug" `
     -RedirectStandardOutput "$env:TEMP\odd_out.txt" -RedirectStandardError "$env:TEMP\odd_err.txt" -PassThru
Start-Sleep -Seconds 5
$crashed = $p.HasExited
if (-not $crashed) { Stop-Process -Id $p.Id }
"crashed: $crashed"
Get-Content "$env:TEMP\odd_out.txt"
Get-Content "$env:TEMP\odd_err.txt"
```

- 성공 기준: `crashed: False`, 출력에 `OpenGL 3.3` 이상 버전이 찍힘, 오류 출력이 비어 있음.
- 창 내용은 하늘색 배경, 녹색 바닥, 주황색 큐브 하나다. 화면은 사용자가 직접 확인하도록 보고에 적는다.

### 5단계: 다른 경로에서도 빌드되는지 (상대 경로 검증)

저장소를 **공백이 들어간 다른 폴더**로 복사해서 다시 빌드한다. 팀원 PC에서 경로가 달라도 되는지 확인하는 단계다.

```powershell
$dst = Join-Path $env:TEMP "odd test copy"
Remove-Item $dst -Recurse -Force -ErrorAction SilentlyContinue
robocopy . $dst /E /XD .vs bin obj .git /NFL /NDL /NJH /NJS | Out-Null
& $msbuild (Join-Path $dst "Oddlands.sln") /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo
Test-Path (Join-Path $dst "bin\Release\Oddlands.exe")
```

### 6단계: 제출용 폴더만으로 실행되는지

Release 실행 폴더만 따로 복사해서 실행한다. 제출할 때 이 폴더만 압축해도 되는지 확인하는 단계다.

```powershell
$pkg = Join-Path $env:TEMP "odd_release_only"
Remove-Item $pkg -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "bin\Release" $pkg -Recurse
$p = Start-Process -FilePath (Join-Path $pkg "Oddlands.exe") -WorkingDirectory $pkg -PassThru
Start-Sleep -Seconds 5
$crashed = $p.HasExited
if (-not $crashed) { Stop-Process -Id $p.Id }
"crashed: $crashed"
```

### 7단계: 정리

```powershell
Remove-Item (Join-Path $env:TEMP "odd test copy"), (Join-Path $env:TEMP "odd_release_only") -Recurse -Force -ErrorAction SilentlyContinue
```

저장소 안의 `bin/`, `obj/`는 지우지 않아도 된다 (git에 안 올라감).

### 보고 형식

아래 표를 채워서 한국어로 보고한다. 실패한 단계가 있으면 표 아래에 오류 메시지 핵심 줄과 추정 원인, 고칠 방법을 적는다.

| 단계 | 내용 | 결과 | 비고 |
|---|---|---|---|
| 1 | MSBuild 찾기 | | VS 버전 |
| 2 | Debug / Release 빌드 | | 오류·경고 수 |
| 3 | 실행 폴더 파일 | | |
| 4 | 실행 5초 | | OpenGL 버전 |
| 5 | 다른 경로 빌드 | | |
| 6 | Release 폴더만으로 실행 | | |
