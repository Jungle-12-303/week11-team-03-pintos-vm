# 9주차 Pintos Threads 구현 계획

## 1. 문서 목적

이 문서는 9주차 Pintos `Project 1: Threads`의 실제 구현 순서와 작업 운영 기준을 정리한 실행 문서다. 협업 원칙과 브랜치 규칙은 [9주차 협업 문서](week9-collaboration.md)에서 관리하고, 이 문서는 "무엇을 어떤 순서로 구현하고, 어떤 기준으로 `dev`에 반영할지"에 집중한다.

## 2. 이번 주 목표

이번 주의 1차 목표는 가능한 많은 코드를 무작정 합치는 것이 아니라, 아래 내용을 팀원 각자가 설명할 수 있을 정도로 이해하면서 구현을 진행하는 것이다.

- `timer_sleep()`이 busy waiting 없이 동작하는 이유
- ready list와 선점이 priority 기준으로 동작하는 이유
- priority donation이 왜 필요한지와 언제 회수되는지
- `thread_mlfqs` 모드에서 priority scheduler와 무엇이 달라지는지

## 3. 성공 기준

이번 주가 잘 진행되고 있다고 판단하는 기준은 다음과 같다.

- 각 담당자는 자신이 맡은 테스트와 관련 함수 흐름을 설명할 수 있다.
- `alarm`, `priority`, `donation`, `mlfqs`를 의존 순서대로 설명할 수 있다.
- `dev`에 병합되는 변경마다 관련 테스트와 구현 의도가 남아 있다.
- 병합된 코드가 왜 통과하는지 팀원이 함께 설명할 수 있다.

## 4. 구현 순서

구현은 아래 순서로 진행한다.

1. `alarm`
2. `priority` 기본 스케줄링
3. `priority` 대기열 우선순위 처리
4. `donation`
5. `mlfqs`
6. 전체 회귀 테스트와 발표 자료 정리

이 순서를 유지하는 이유는 의존성 때문이다.

- `alarm-priority`는 결국 ready list와 선점 흐름에 영향을 받는다.
- `donation`은 `priority` 기본 동작이 안정적이어야 디버깅이 가능하다.
- `mlfqs`는 별도 스케줄링 규칙이므로 `priority`/`donation`과 분리해서 보는 편이 안전하다.

## 5. 초기 작업 분장

초기 `alarm` 작업은 테스트 기준으로 다음처럼 나눈다.

| 담당자 | 초기 담당 테스트 | 중점 확인 포인트 |
|------|------|------|
| `changwon` | `alarm-single`, `alarm-multiple` | 기본 sleep/wakeup 흐름, wakeup tick 계산 |
| `hyungmin` | `alarm-simultaneous`, `alarm-priority` | 같은 tick에서 여러 thread를 깨우는 흐름, wakeup 이후 priority 반영 |
| `donghyun` | `alarm-zero`, `alarm-negative` | 경계값 처리, `timer_sleep()` API 동작 |

초기 분장은 테스트 기준으로 나눴지만, 실제 구현은 같은 설계를 공유한다. 따라서 `alarm` 병합 전에는 `pintos/devices/timer.c`, `pintos/threads/thread.c`, `pintos/include/threads/thread.h` 흐름을 함께 확인한다.

## 6. 이후 작업 분할 기준

이후 업무도 비슷한 테스트끼리 묶되, 실제 수정 파일과 함수 연관성을 함께 고려해 나눈다.

| 작업 묶음 | 관련 테스트 | 핵심 파일 | 병합 전 확인 포인트 |
|------|------|------|------|
| `alarm` 전체 | `alarm-*` | `pintos/devices/timer.c`, `pintos/threads/thread.c`, `pintos/include/threads/thread.h` | sleeping list, wakeup tick, interrupt context |
| `priority` 기본 | `priority-change`, `priority-fifo`, `priority-preempt` | `pintos/threads/thread.c` | ready list 정렬, 선점, `thread_set_priority()` 후 yield |
| `priority` waiters | `priority-sema`, `priority-condvar` | `pintos/threads/synch.c`, `pintos/threads/thread.c` | highest-priority waiter 선택 |
| `donation` 기본 | `priority-donate-one`, `priority-donate-multiple`, `priority-donate-multiple2`, `priority-donate-lower` | `pintos/threads/thread.c`, `pintos/threads/synch.c`, `pintos/include/threads/thread.h`, `pintos/include/threads/synch.h` | base/effective priority 분리, donation 회수 |
| `donation` 전파 | `priority-donate-nest`, `priority-donate-chain`, `priority-donate-sema` | `pintos/threads/thread.c`, `pintos/threads/synch.c` | nested donation, waiting lock 추적 |
| `mlfqs` 계산 | `mlfqs-load-1`, `mlfqs-load-60`, `mlfqs-load-avg`, `mlfqs-recent-1` | `pintos/threads/thread.c`, `pintos/devices/timer.c`, `pintos/include/threads/thread.h` | `load_avg`, `recent_cpu`, 갱신 주기 |
| `mlfqs` 동작 | `mlfqs-fair-2`, `mlfqs-fair-20`, `mlfqs-block`, `mlfqs-nice-2`, `mlfqs-nice-10` | `pintos/threads/thread.c`, `pintos/devices/timer.c` | priority 재계산, ready list 재정렬, `nice` 반영 |

## 7. `dev` 병합 게이트

`dev`에 올리는 PR은 최소한 아래 조건을 만족해야 한다.

- 관련 테스트 이름이 명시되어 있다.
- 수정한 파일과 핵심 함수가 정리되어 있다.
- 통과한 테스트와 남은 테스트가 구분되어 있다.
- 구현 의도와 설계 가정이 적혀 있다.
- 작성자가 코드 흐름을 설명할 수 있다.
- 팀원 1명 이상이 리뷰한다.

다음 경우에는 병합하지 않는다.

- 테스트는 통과했지만 왜 통과하는지 설명하지 못하는 경우
- 관련 테스트 외 회귀 가능성이 큰데 확인이 없는 경우
- `thread.c`, `synch.c`, `timer.c`의 결합 영향이 큰데 단독 판단으로 올린 경우

## 8. 테스트 실행 원칙

테스트는 아래 원칙으로 운영한다.

- 한 번에 많은 테스트를 섞기보다 작업 묶음 단위로 점검한다.
- 실패를 공유할 때는 테스트 이름, 재현 명령, 기대 결과, 실제 결과를 함께 남긴다.
- `priority-donate-*`가 실패하면 먼저 `priority-preempt`, `priority-fifo`, `priority-sema`가 안정적인지 본다.
- `mlfqs-*`는 `priority`/`donation`과 분리해서 점검한다.
- 통과 결과만 적지 말고, 어떤 설계 변경이 어떤 테스트를 통과시켰는지 남긴다.

## 9. 충돌 가능 파일

아래 파일은 여러 작업 묶음이 겹칠 가능성이 높으므로 병렬 작업 시 특히 주의한다.

- `pintos/devices/timer.c`
- `pintos/threads/thread.c`
- `pintos/threads/synch.c`
- `pintos/include/threads/thread.h`
- `pintos/include/threads/synch.h`

개인 작업이라도 위 파일을 크게 바꾸는 경우에는 병합 전에 팀원과 함께 흐름을 확인한다.

## 10. 일일 점검 항목

스크럼이나 코어타임에서는 최소한 아래를 확인한다.

- 오늘 `dev`에 올릴 수 있는 PR이 있는가
- 아직 실패 중인 테스트는 무엇인가
- 실패 원인을 함수 단위로 설명할 수 있는가
- 팀 전체가 이해하지 못한 개념이 남아 있는가
- 발표 자료와 WIL에 남길 근거가 정리되고 있는가

## 11. 문서 갱신 원칙

이 문서는 고정 계획표가 아니라 실행 상태에 따라 갱신하는 문서다.

- 초기 분장이 끝나면 다음 작업 묶음 담당을 추가한다.
- 테스트 통과 현황이 바뀌면 병합 기준과 리스크를 갱신한다.
- 구현 순서가 바뀌면 이유를 남긴다.
