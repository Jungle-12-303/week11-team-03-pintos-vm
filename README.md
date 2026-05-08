# Pintos Virtual Memory 협업 저장소

이 저장소는 4인 팀이 KAIST Pintos `Project 3: Virtual Memory`를 함께 구현하고 검증하기 위한 작업 공간입니다. 루트 디렉터리에는 팀 공통 문서와 개발환경 설정을 두고, 실제 Pintos 소스 코드는 `pintos/` 아래에서 관리합니다.

현재 주 작업 범위는 11-12주차 `Project 3: Virtual Memory`입니다. 기존 9주차 `Project 1: Threads`와 10주차 `Project 2: User Programs` 문서는 참고용으로만 유지하며, 이번 주차의 협업 기준과 구현 순서는 11-12주차 문서를 따릅니다.

## 빠른 시작

1. Docker Desktop을 설치합니다.
2. VSCode에서 이 저장소를 엽니다.
3. `Dev Containers: Reopen in Container`로 컨테이너 환경에 들어갑니다.
4. 필요하면 컨테이너 셸에서 `source /workspaces/week11-team-03-pintos-vm/pintos/activate`를 실행합니다.

자세한 환경 구성 방법은 [개발환경 구축 가이드](docs/dev-environment.md)를 참고하세요.

## 주요 경로

| 경로 | 설명 |
|------|------|
| `README.md` | 저장소 개요와 협업 진입 문서 |
| `docs/dev-environment.md` | Docker, VSCode DevContainer 기반 개발환경 안내 |
| `docs/week11-12-collaboration.md` | 11-12주차 Virtual Memory 협업 기준 |
| `docs/week11-12-implementation-plan.md` | 11-12주차 Virtual Memory 2주 구현 계획 |
| `docs/archive/` | 9-10주차 공식 협업/구현 문서 아카이브 |
| `docs/kaist-pintos-gitbook/` | KAIST Pintos GitBook 로컬 참고 문서 |
| `docs/reference/gitbook-reference-links.md` | KAIST Pintos GitBook 원문 링크 모음 |
| `pintos/README.md` | Pintos 원본 안내 문서 |
| `pintos/threads/` | Project 1 thread/synchronization 관련 코드 |
| `pintos/userprog/` | Project 2 user process, syscall, process lifecycle 관련 코드 |
| `pintos/vm/` | Project 3 virtual memory 관련 코드 |
| `pintos/tests/threads/` | Project 1 회귀 테스트 코드 |
| `pintos/tests/userprog/` | Project 2 회귀 테스트 코드 |
| `pintos/tests/vm/` | Project 3 VM 테스트 코드 |
| `.devcontainer/` | 팀 공통 컨테이너 개발환경 설정 |
| `.gitattributes` | tracked text 파일 LF 줄바꿈 정책 |

## 11-12주차 작업 원칙

- 4명이 동시에 같은 코드와 테스트 로그를 보며 페어 프로그래밍 방식으로 작업합니다.
- 명칭은 페어 프로그래밍을 사용하지만, 실제 운영은 Driver 1명과 공동 검토자 3명이 함께 진행하는 4인 공동 코딩입니다.
- 장기 개인 분업보다 공통 설계, 작은 변경, 빠른 테스트 재현을 우선합니다.
- `pintos/vm/`, `pintos/include/vm/`, `pintos/userprog/process.c`, `pintos/userprog/exception.c`, `pintos/userprog/syscall.c`, `pintos/include/threads/thread.h` 변경은 4명이 함께 흐름을 확인합니다.
- Project 3 구현 중에도 Project 1/2 회귀 테스트를 유지합니다.
- Extra COW는 VM 필수 범위가 안정된 뒤에만 판단합니다.

## 줄바꿈 정책

이 저장소의 tracked text 파일은 LF 줄바꿈으로 관리합니다.

- `.gitattributes`의 `* text=auto eol=lf` 정책을 기준으로 합니다.
- 새 문서와 코드 변경은 UTF-8 without BOM과 LF 줄바꿈을 사용합니다.
- 커밋 전 `git ls-files --eol --modified --others --exclude-standard`로 `w/crlf` 파일이 없는지 확인합니다.
- 커밋 전 `git diff --check`로 공백과 줄바꿈 문제를 확인합니다.

## 참고 문서

- [11-12주차 Virtual Memory 협업 문서](docs/week11-12-collaboration.md)
- [11-12주차 Virtual Memory 구현 계획](docs/week11-12-implementation-plan.md)
- [개발환경 구축 가이드](docs/dev-environment.md)
- [Pintos 안내 문서](pintos/README.md)
- [KAIST Pintos Manual](https://casys-kaist.github.io/pintos-kaist/)
