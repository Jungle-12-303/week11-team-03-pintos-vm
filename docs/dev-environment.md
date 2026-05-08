# Pintos 개발환경 구축 가이드

이 문서는 **Windows**와 **macOS** 사용자가 Docker와 VSCode DevContainer 기능을 활용하여 Pintos OS 프로젝트 개발 환경을 빠르게 구성할 수 있도록 안내합니다.

[**주의**]
* `ubuntu:22.04` 버전은 충분한 테스트와 검증이 되지 않았습니다. 이 점을 주의해서 사용하시기 바랍니다.

[**참고**] 
* pintos 도커 환경은 `64비트 기반 X86-64` 기반의 `ubuntu:22.04` 버전을 사용합니다.
   * kaist-pintos는 오리지널 pintos와 달리 64비트 환경을 지원합니다.
   * 이번 도커 환경은 ubuntu 22.04를 지원하여 vscode의 최신 버전에서 원격 연결이 안되는 문제를 해결하였습니다.
* pintos 도커 환경은 kaist-pintos에서 추천하는 qemu 에뮬레이터를 설치하고 사용합니다. 
* pintos 도커 환경은 교육 일정 전반에서 공통으로 사용할 수 있는 기본 개발 환경입니다.
* 기존 도커 환경과 달리 `vscode`와 통합된 디버깅 환경(F5로 시작하는)을 제공하지 않습니다. 디버깅이 필요한 경우 `gdb`를 사용하세요. 
* VSCode에서 터미널을 열면 자동으로 현재 워크스페이스의 `pintos/activate`를 찾아 실행합니다.

---

## 1. Docker란 무엇인가요?

**Docker**는 애플리케이션을 어떤 컴퓨터에서든 **동일한 환경에서 실행**할 수 있게 도와주는 **가상화 플랫폼**입니다.  

Docker는 다음 구성요소로 이루어져 있습니다:

- **Docker Engine**: 컨테이너를 실행하는 핵심 서비스
- **Docker Image**: 컨테이너 생성에 사용되는 템플릿 (레시피 📃)
- **Docker Container**: 이미지를 기반으로 생성된 실제 실행 환경 (요리 🍜)

### ✅ AWS EC2와의 차이점

| 구분 | EC2 같은 VM | Docker 컨테이너 |
|------|-------------|-----------------|
| 실행 단위 | OS 포함 전체 | 애플리케이션 단위 |
| 실행 속도 | 느림 (수십 초 이상) | 매우 빠름 (거의 즉시) |
| 리소스 사용 | 무거움 | 가벼움 |

---

## 2. VSCode DevContainer란 무엇인가요?

**DevContainer**는 VSCode에서 Docker 컨테이너를 **개발 환경**처럼 사용할 수 있게 해주는 기능입니다.

- 코드를 실행하거나 디버깅할 때 **컨테이너 내부 환경에서 동작**
- 팀원 간 **환경 차이 없이 동일한 개발 환경 구성** 가능
- `.devcontainer` 폴더에 정의된 설정을 VSCode가 읽어 자동 구성

---

## 3. Docker Desktop 설치하기

1. Docker 공식 사이트에서 설치 파일 다운로드:  
   👉 [https://www.docker.com/products/docker-desktop](https://www.docker.com/products/docker-desktop)

2. 설치 후 Docker Desktop 실행  
   - Windows: Docker 아이콘이 트레이에 떠야 함  
   - macOS: 상단 메뉴바에 Docker 아이콘 확인

---

## 4. 프로젝트 폴더 구조

```text
week11-team-03-pintos-vm/
├── .devcontainer/
│   ├── devcontainer.json      # VSCode에서 컨테이너 환경 설정
│   └── Dockerfile             # pintos 개발 환경 도커 이미지 정의
│
├── docs/
│   ├── dev-environment.md     # 현재 문서
│   ├── week11-12/             # 11-12주차 협업 및 구현 계획
│   └── archive/               # 9-10주차 공식 문서 아카이브
│
├── pintos/
│   ├── threads                # Thread 프로젝트 폴더
│   ├── userprog               # User program 프로젝트 폴더
│   └── vm                     # Virtual memory 프로젝트 폴더
│
└── README.md                  # 저장소 안내 문서
```
---

## 5. VSCode에서 해당 프로젝트 폴더 열기

1. VSCode를 실행
2. `파일 → 폴더 열기`로 해당 프로젝트 루트 폴더를 선택

---

## 6. 개발 컨테이너: 컨테이너에서 열기

1. VSCode에서 `Ctrl+Shift+P` (Windows/Linux) 또는 `Cmd+Shift+P` (macOS)를 누릅니다.
2. 명령어 팔레트에서 `Dev Containers: Reopen in Container`를 선택합니다.
3. 이후 컨테이너가 자동으로 실행되고 빌드됩니다. 처음 컨테이너를 열면 빌드하는 시간이 오래걸릴 수 있습니다. 빌드 후, 프로젝트가 **컨테이너 안에서 실행됨**.

---

## 7. C 파일에 브레이크포인트 설정 후 디버깅 (F5)
pintos 랩에서는 VSCode 기반의 F5 디버깅을 지원하지 않습니다. 디버깅이 필요하면 `gdb`를 사용하세요.
