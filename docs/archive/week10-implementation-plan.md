# 10주차 Pintos User Programs 구현 계획

## 1. 문서 목적

이 문서는 10주차 Pintos `Project 2: User Programs`의 구현 순서와 작업 운영 기준을 정리한 실행 문서다. 9주차 `threads` 구현을 기반으로, 사용자 프로그램이 커널 제어 아래 안전하게 실행되도록 만드는 흐름에 집중한다.

협업 원칙과 브랜치 규칙은 [10주차 협업 문서](week10-collaboration.md)에서 관리하고, 이 문서는 10주차에 무엇을 어떤 순서로 구현하고 어떤 테스트 기준으로 점검할지에 집중한다.

## 2. 학습 목표

10주차의 학습 목표는 다음과 같다.

- ELF loader와 사용자 프로그램 실행 경로를 이해한다.
- 사용자 프로그램의 argument passing을 구현한다.
- system call dispatcher와 필수 system call을 구현한다.
- file descriptor table과 file system 접근 흐름을 구현한다.
- user/kernel address validation을 통해 커널이 잘못된 사용자 포인터로부터 안전하게 동작하도록 만든다.
- process wait/exit/exec/fork 흐름을 구현하고 parent-child 관계를 설명할 수 있게 한다.

## 3. 학습 키워드

- User Program
- User Mode
- Kernel Mode
- System Call
- Virtual Memory Layout
- ELF Loader
- File Descriptor
- User Memory Validation
- Process Lifecycle

## 4. 이번 주 목표

이번 주의 1차 목표는 `Project 2: User Programs`의 필수 테스트를 통과할 수 있는 구현 기반을 만드는 것이다. Extra 과제는 필수 구현 이후 별도 판단한다.

테스트 통과만이 목표가 아니라, 팀원이 아래 흐름을 설명할 수 있어야 한다.

- `process_create_initd()`에서 user process가 만들어지는 흐름
- `process_exec()`와 `load()`가 실행 파일을 열고 ELF segment를 적재하는 흐름
- `_start(argc, argv)`가 호출된 것처럼 stack과 register를 준비하는 흐름
- `syscall_entry.S`가 저장한 `intr_frame`에서 syscall 번호와 인자를 읽는 방식
- user pointer를 검증하지 않으면 커널이 위험해지는 이유
- fd table이 `intr_frame`이 아니라 process/thread 상태에 있어야 하는 이유
- 부모가 자식의 load/exit를 기다리는 방식

## 5. 구현 순서

User Programs 구현은 아래 순서로 진행한다.

1. 임시 실행 흐름 유지
2. argument passing
3. 최소 syscall dispatcher
4. `SYS_HALT`, `SYS_EXIT`, `SYS_WRITE(fd=1)`
5. user memory validation 강화
6. process exit 상태 저장
7. file syscall과 file descriptor table
8. 정식 process wait/exit 동기화
9. exec/fork와 복합 프로세스 테스트
10. 전체 회귀 테스트와 발표 자료 정리

이 순서를 유지하는 이유는 user program 실행 흐름의 의존성 때문이다.

- 정식 wait 전에는 `process_wait()`가 즉시 반환하지 않도록 임시 대기가 있어야 사용자 프로그램 실행 흐름을 관찰할 수 있다.
- argument passing이 없으면 `argc/argv`가 잘못되어 `halt`, `exit` 같은 작은 테스트도 `_start()` 또는 `main()` 진입 초기에 page fault를 낼 수 있다.
- `SYS_WRITE(fd=1)`은 출력 디버깅에 유용하지만, 사용자 프로그램이 정상적으로 시작되기 위한 선행 조건은 argument passing이다.
- syscall dispatcher와 기본 syscall은 argument passing 이후에 작게 열어 두고, 이후 테스트 범위에 맞춰 확장한다.
- bad pointer와 boundary 테스트는 syscall 인자를 읽는 경로가 생긴 뒤 user pointer 검증 정책을 안정화해야 한다.
- file syscall은 fd table, file system lock, stdin/stdout 예외 처리가 함께 필요하다.
- wait/exec/fork는 parent-child 관계, load 완료, exit 완료, exit status 전달이 연결된 뒤에 구현하는 편이 안전하다.

## 6. 작업 분할 기준

작업은 user process 실행 흐름과 테스트 묶음을 기준으로 나눈다.

| 작업 묶음 | 관련 테스트 | 핵심 파일 | 병합 전 확인 포인트 |
|------|------|------|------|
| 임시 실행 흐름 유지 | 초기 실행 관찰 | `pintos/userprog/process.c` | `process_wait()` 즉시 반환 방지, 임시 구현 표시 |
| argument passing | `args-none`, `args-single`, `args-multiple`, `args-many`, `args-dbl-space` | `pintos/userprog/process.c` | command line 토큰화, executable name 분리, stack alignment, `RDI/RSI/RSP` |
| 최소 syscall 기반 | `halt`, `exit`, stdout 출력 | `pintos/userprog/syscall.c` | syscall dispatcher, `SYS_HALT`, `SYS_EXIT`, `SYS_WRITE(fd=1)` |
| user memory validation | `*-bad-ptr`, `*-boundary`, `bad-*` | `pintos/userprog/syscall.c`, 필요 시 `pintos/userprog/exception.c` | user/kernel address 구분, page mapping 확인, 실패 시 `exit(-1)` |
| process exit 상태 | `exit`, `wait-killed` 기반 | `pintos/userprog/syscall.c`, `pintos/userprog/process.c`, `pintos/include/threads/thread.h` | exit status 저장, 종료 메시지, 자원 정리 |
| file syscall/fd table | `create-*`, `open-*`, `read-*`, `write-*`, `close-*` | `pintos/userprog/syscall.c`, `pintos/include/threads/thread.h` | fd table, stdin/stdout, file system lock, invalid fd |
| wait/exec | `wait-*`, `exec-*` | `pintos/userprog/process.c`, `pintos/userprog/syscall.c` | parent-child 관계, load 완료 동기화, exit status 전달 |
| fork/multi | `fork-*`, `multi-*` | `pintos/userprog/process.c`, `pintos/userprog/syscall.c` | address space 복제, fd 복제, child resource cleanup |

## 7. 테스트 운영 순서

테스트는 아래 순서로 좁게 확인한 뒤 범위를 넓힌다.

1. 빌드 확인
   - `pintos/userprog`에서 kernel과 user test binary가 컴파일되는지 확인한다.

2. 초기 실행 흐름 확인
   - 정식 wait 구현 전에는 `process_wait()`가 즉시 반환하지 않는지만 확인한다.
   - 이 단계의 목적은 테스트 통과가 아니라 user process가 생성되고 load되는 흐름을 관찰하는 것이다.

3. argument passing 확인
   - `args-none`
   - `args-single`
   - `args-multiple`
   - `args-many`
   - `args-dbl-space`

4. 기본 system call 확인
   - `halt`
   - `exit`
   - stdout 출력

5. 잘못된 포인터와 boundary 확인
   - `*-bad-ptr`
   - `*-boundary`
   - `bad-read`, `bad-write`, `bad-jump` 계열

6. 파일 system call 확인
   - `create-*`
   - `open-*`
   - `read-*`
   - `write-*`
   - `close-*`

7. process lifecycle 확인
   - `wait-*`
   - `exec-*`
   - `fork-*`
   - `multi-*`
   - `rox-*`

테스트 실패를 공유할 때는 테스트 이름, 재현 명령, 기대 결과, 실제 결과, 최근 수정 파일을 함께 남긴다.

## 8. 임시 구현 관리

초기 디버깅을 위해 임시 구현을 둘 수 있다. 단, 아래 조건을 지킨다.

- 임시 구현임을 코드 주석과 PR 설명에 명확히 남긴다.
- 어떤 테스트 관찰을 위해 필요한 임시 구현인지 적는다.
- 정식 구현으로 대체할 작업 묶음을 이 문서 또는 이슈에 남긴다.
- 임시 구현을 포함한 커밋 메시지에는 `임시` 또는 `temporary`에 해당하는 의미가 드러나게 한다.

예시:

- 정식 wait 전 `process_wait()`가 즉시 반환하지 않도록 임시 sleep loop를 둔다.
- argument passing 이후 출력 확인을 위해 `SYS_WRITE(fd=1)`만 먼저 구현한다.

임시 구현은 최종 제출 전에 정식 구현으로 대체해야 한다.

## 9. 충돌 가능 파일

아래 파일은 여러 작업 묶음이 겹칠 가능성이 높으므로 병렬 작업 시 특히 주의한다.

- `pintos/userprog/process.c`
- `pintos/userprog/syscall.c`
- `pintos/userprog/exception.c`
- `pintos/include/userprog/process.h`
- `pintos/include/userprog/syscall.h`
- `pintos/include/threads/thread.h`
- `pintos/threads/thread.c`

개인 작업이라도 위 파일을 크게 바꾸는 경우에는 병합 전에 팀원과 함께 흐름을 확인한다.

`pintos/filesys/filesys.c`와 `pintos/filesys/file.c`는 file syscall 구현 시 인터페이스를 자주 참고하지만, Project 2에서는 가능하면 내부 구현을 직접 바꾸지 않는다. 파일 시스템 동시성은 userprog 계층의 lock과 fd table 설계로 먼저 해결한다.

## 10. 병합 게이트

`dev`에 올리는 PR은 최소한 아래 조건을 만족해야 한다.

- 관련 테스트 이름이 명시되어 있다.
- 수정한 파일과 핵심 함수가 정리되어 있다.
- 통과한 테스트와 남은 테스트가 구분되어 있다.
- 구현 의도와 설계 가정이 적혀 있다.
- user pointer, fd table, process lifecycle 중 어느 상태를 소유하는지 설명되어 있다.
- 임시 구현이 있으면 임시 범위와 대체 계획이 적혀 있다.
- 작성자가 코드 흐름을 설명할 수 있다.
- 팀원 2명 이상이 리뷰한다.

다음 경우에는 병합하지 않는다.

- 테스트는 통과했지만 왜 통과하는지 설명하지 못하는 경우
- 관련 테스트 외 회귀 가능성이 큰데 확인이 없는 경우
- user pointer 검증 실패, lock 미해제, file descriptor 누수 가능성이 있는데 확인하지 않은 경우
- process/thread 구조체에 상태를 추가했지만 lifecycle과 cleanup 책임이 설명되지 않은 경우

## 11. 주간 공유와 WIL 기록

공지 기준으로 주간 공유 발표와 WIL은 10주차 산출물에 포함된다.

주간 공유 발표에는 아래 내용을 포함한다.

- 프로젝트 팀 구성
- Project 2 구현 현황
- 주요 트러블슈팅
- 남은 테스트와 리스크
- 개인별 회고

WIL에는 단순히 통과한 테스트 목록만 적지 않는다. 아래 내용을 함께 남긴다.

- 새로 이해한 개념
- 실패 테스트의 원인
- 디버깅 과정에서 확인한 근거
- 구현 선택의 이유
- 다음 주차로 넘길 리스크

## 12. 완료 기준

10주차 작업이 잘 진행되고 있다고 볼 수 있는 기준은 다음과 같다.

- 팀원은 user program 실행 경로를 `process_create_initd()`부터 syscall 처리까지 설명할 수 있다.
- 팀원은 argument passing의 stack layout과 `RDI/RSI` 전달 규칙을 설명할 수 있다.
- 팀원은 user pointer validation의 필요성과 실패 처리 방식을 설명할 수 있다.
- 팀원은 fd table과 file system lock의 필요성을 설명할 수 있다.
- 팀원은 wait/exit/exec/fork의 process lifecycle을 테스트와 연결해 설명할 수 있다.
- `dev`에 병합되는 변경은 관련 테스트와 설계 설명이 함께 남는다.
- 코드, 테스트, 문서, 발표 자료가 같은 근거를 공유한다.
