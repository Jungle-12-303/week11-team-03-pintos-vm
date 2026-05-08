# 10주차 Pintos User Programs 협업 문서

## 1. 문서 목적

이 문서는 10주차 Pintos `Project 2: User Programs`를 3인 팀이 함께 진행할 때의 협업 기준을 정리한 공식 문서다. 9주차 `threads` 구현을 기반으로 하나의 팀 코드베이스를 유지하면서 user program 실행 경로, system call, file descriptor, user/kernel address validation을 함께 구현한다.

`local/` 폴더의 자료는 개인 참고용으로 두고, 팀이 실제로 합의해 따를 내용은 `docs/` 아래 문서로 관리한다.

구체적인 구현 순서와 병합 기준은 [10주차 구현 계획 문서](week10-implementation-plan.md)에서 별도로 관리한다.

## 2. 10주차 학습 목표

이번 주의 목표는 `Project 2: User Programs`의 필수 테스트를 통과하는 것뿐 아니라, 팀원 각자가 아래 흐름을 설명할 수 있게 되는 것이다.

- ELF loader가 사용자 프로그램을 메모리에 적재하는 흐름
- `_start(argc, argv)`와 argument passing의 stack/register 구성
- user mode에서 kernel mode로 system call이 들어오는 흐름
- `intr_frame`의 syscall 번호와 인자 레지스터 사용 방식
- file descriptor table의 소유권과 lifecycle
- user pointer 검증이 필요한 이유와 실패 처리 방식
- parent-child process 관계, wait/exit/exec/fork 동기화

## 3. 협업 원칙

- 팀 최종 결과물은 하나의 공용 코드베이스로 관리한다.
- 개인별 branch에서 작업하고, `main`에는 직접 push하지 않는다.
- `dev` 또는 통합 브랜치로 PR을 올릴 때는 구현 의도, 관련 테스트, 남은 리스크를 함께 적는다.
- 테스트 통과만으로 병합하지 않고, 코드 작성자가 실행 흐름과 실패 시나리오를 설명할 수 있어야 한다.
- 임시 구현은 반드시 임시임을 표시하고, 정식 구현으로 대체할 계획을 남긴다.
- AI 도구는 개념 정리, 문서 요약, 디버깅 가설 정리에 활용하되, 이해 없이 코드를 복사해 병합하지 않는다.
- 최종 코드와 설명 책임은 작성자에게 있다.

## 4. 이슈 보드 운영 방식

10주차 이슈는 GitHub Projects 보드를 기준으로 관리한다.

Project URL: https://github.com/orgs/Jungle-12-303/projects/4

- 공통 이슈: 주간 목표, AI 활용 원칙, WIL, 협업 규칙 점검
- 학습 이슈: KAIST Project 2 문서, FAQ, user memory, syscall, file descriptor 학습
- 구현 이슈: argument passing, syscall, pointer validation, fd table, wait/exec/fork
- 테스트 이슈: `args-*`, `halt`, `exit`, file syscall, bad pointer, wait/exec/fork 테스트

이슈는 사람 수 기준으로 나누기보다, 관련 함수와 실패 원인이 비슷한 테스트끼리 묶는다.

## 5. 작업 분할 기준

10주차 작업은 user program 실행 흐름의 의존성에 따라 나눈다.

| 작업 묶음 | 관련 테스트 | 중점 확인 포인트 |
|------|------|------|
| 임시 실행 흐름 유지 | 초기 실행 관찰 | 정식 wait 전 `process_wait()` 즉시 반환 방지 |
| argument passing | `args-none`, `args-single`, `args-multiple`, `args-many`, `args-dbl-space` | command line 토큰화, stack 배치, 8바이트 정렬, `RDI/RSI` 설정 |
| 최소 syscall 기반 | `halt`, `exit`, stdout 출력 | syscall dispatcher, `SYS_HALT`, `SYS_EXIT`, `SYS_WRITE(fd=1)` |
| user memory validation | bad pointer, boundary 계열 | user/kernel address 구분, page mapping 확인, 자원 누수 없는 종료 |
| file syscall/fd table | create/open/read/write/close 계열 | file system lock, fd table, stdin/stdout, invalid fd 처리 |
| process lifecycle | wait/exec/fork/multi 계열 | parent-child 관계, load/exit 동기화, exit status 전달 |

`SYS_WRITE(fd=1)`은 디버깅 출력 확인에 유용하지만, 빈 구현 상태에서 사용자 프로그램이 안정적으로 `main()`에 들어가려면 argument passing이 먼저 필요하다. 따라서 기본 syscall은 argument passing 이후 작게 열고, 이후 pointer validation과 fd table을 붙여 확장한다.

## 6. PR, 리뷰, 병합 기준

모든 PR에는 최소한 아래 내용이 들어가야 한다.

- 관련 이슈 또는 테스트 이름
- 수정한 파일과 핵심 함수
- 통과한 테스트와 아직 남은 테스트
- 구현 의도 또는 설계 가정
- user pointer, fd table, process lifecycle 중 어떤 상태를 소유하거나 변경했는지
- 남아 있는 리스크 또는 확인이 필요한 부분
- 임시 구현이 포함된 경우 임시 범위와 제거 또는 대체 계획

`dev` 병합 기준은 다음과 같다.

- 관련 테스트가 재현 가능하게 통과해야 한다.
- 작성자가 수정 이유와 동작 원리를 설명할 수 있어야 한다.
- 팀원 1명 이상이 리뷰해야 한다.
- `process.c`, `syscall.c`, process/thread 구조체, fd table 변경은 가능하면 함께 보고 병합한다.
- user pointer 검증, process wait/exit 동기화, file descriptor 소유권처럼 userprog 핵심 개념이 걸린 변경은 테스트 결과와 실패 시나리오를 함께 설명한다.

다음 경우에는 병합하지 않는다.

- 테스트는 통과했지만 이유를 설명하지 못하는 경우
- 관련 테스트 외 회귀 영향이 의심되는데 확인이 없는 경우
- 임시 구현이 정식 구현처럼 설명된 경우
- AI 제안이나 외부 코드를 그대로 가져와 이해 없이 붙인 경우

## 7. 커밋 메시지와 PR 작성 규칙

- 커밋은 한 가지 의도만 담고, 기능명이나 테스트명을 포함해 작성한다.
- 커밋 메시지는 `<type>: <한국어 제목>` 형식을 따른다.
- 커밋 제목에는 스코프를 사용하지 않는다. 영향 범위는 제목과 PR 본문에서 설명한다.
- 허용 type은 `feat`, `fix`, `refactor`, `docs`, `test`, `chore`, `style`, `perf`, `build`, `ci`, `revert`이다.
- 제목은 한국어 한 줄로 작성하고 마침표로 끝내지 않는다.
- 제목은 행위가 아니라 변경 결과를 구체적으로 설명한다.
- `수정`, `작업`, `변경`, `업데이트`, `update`, `fix`처럼 의미가 약한 표현만 단독으로 쓰지 않는다.
- 커밋 예시: `feat: 최소 시스템 콜 디스패처를 추가`
- 커밋 예시: `feat: 사용자 프로그램 인자를 스택에 전달`
- 커밋 예시: `fix: 사용자 버퍼 경계 검증을 보완`
- 커밋 예시: `docs: 10주차 User Programs 구현 계획을 정리`

## 8. 테스트 운영 원칙

테스트는 "끝났는지 체크하는 용도"가 아니라 "설계 가설이 맞는지 확인하는 도구"로 사용한다.

- 실패 테스트를 공유할 때는 테스트 이름, 재현 명령, 기대 결과, 실제 결과, 최근 수정 내용을 함께 남긴다.
- 한 번에 여러 테스트가 깨지면 의존성이 앞선 단계부터 본다.
- `args-*`가 실패할 때는 syscall보다 argument passing, stack alignment, `RDI/RSI` 설정을 먼저 본다.
- `halt` 또는 `exit`가 실패할 때도 argument passing, `process_wait()`의 즉시 반환 여부, `SYS_HALT`, `SYS_EXIT`, `SYS_WRITE(fd=1)` 흐름을 함께 확인한다.
- bad pointer 계열이 실패할 때는 user pointer 검증과 자원 해제 순서를 함께 본다.
- file syscall 계열이 실패할 때는 fd table, file system lock, invalid fd 처리를 함께 본다.
- wait/exec/fork 계열이 실패할 때는 parent-child lifecycle과 semaphore 동기화를 함께 본다.
- 관련 없는 대규모 수정으로 여러 테스트를 동시에 건드리지 않는다.

## 9. 소통과 학습 공유

- 막힌 테스트는 오래 끌지 말고 바로 공유한다.
- 코어타임이나 스크럼에서는 "무엇을 바꿨는지"보다 "왜 그렇게 설계했는지"를 우선 설명한다.
- 임시 구현을 추가한 경우 언제 제거할지 같이 공유한다.
- 이해가 부족한 상태의 병합보다, 늦더라도 팀 전체가 설계를 따라갈 수 있는 병합을 우선한다.
- WIL에는 단순 결과보다 배운 개념, 실패 원인, 수정 근거를 남긴다.

## 10. 주간 공유와 제출물

공지 기준으로 주간 공유 발표와 WIL은 10주차 산출물에 포함된다.

- 주간 공유 발표는 팀별 하나의 문서와 하나의 노트북으로 준비한다.
- 발표는 개인별 2분씩 진행한다.
- 발표에는 프로젝트 팀 구성, 구현 및 트러블슈팅, 프로젝트 회고를 포함한다.
- WIL은 이번 주 작성한 블로그 URL을 `WEEK10` 태그와 함께 등록한다.
- 차주 발제 시간에는 10주차 동료피드백을 진행한다.
