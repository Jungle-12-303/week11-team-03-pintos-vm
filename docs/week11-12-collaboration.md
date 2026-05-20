# 11-12주차 Pintos Virtual Memory 협업 문서

## 1. 문서 목적

이 문서는 11-12주차 Pintos `Project 3: Virtual Memory` 협업 기준을 정리한다. 세부 운영 기록과 최신 날짜별 계획은 `local/study_plan/week11-12/team-collaboration.md`를 기준으로 하며, 이 문서는 `docs/`에서 확인할 수 있는 짧은 요약 문서로 유지한다.

이번 주차의 목표는 VM 테스트 통과만이 아니라 팀원이 아래 흐름을 함께 설명할 수 있는 상태를 만드는 것이다.

- SPT가 page fault 처리의 기준 데이터가 되는 이유
- lazy loading, stack growth, mmap, eviction/swap이 `struct page`, `struct frame`, SPT와 연결되는 방식
- mmap file, frame, swap slot, aux 같은 자원의 생성 시점과 해제 시점
- Project 2 process, syscall, fd table 구현이 VM 위에서 회귀 없이 동작해야 하는 이유

## 2. 현재 운영 방식

작업은 담당자별 장기 분업이 아니라 VM 구현 흐름과 테스트 묶음을 기준으로 진행한다. 2026-05-11과 2026-05-12는 4명이 한 화면에서 공동 구현했고, 2026-05-13부터는 2명:2명 페어 프로그래밍으로 나누어 진행한다.

- 4인 세션: Driver 1명과 공동 검토자 3명이 설계, cleanup, 테스트 조건을 함께 확인한다.
- 2인 페어: Driver 1명과 Navigator 1명이 작은 구현 단위를 맡고, 페어 간 변경 범위와 실패 지점을 공유한다.
- 역할은 30-60분 단위로 교체한다.
- `pintos/vm/`, `pintos/include/vm/`, `pintos/userprog/process.c`, `pintos/userprog/exception.c`, `pintos/userprog/syscall.c`, `pintos/include/threads/thread.h` 변경은 흐름을 함께 확인한다.
- 한 명만 이해하는 구조체 필드나 cleanup 경로는 병합하지 않는다.

## 3. 브랜치와 병합

현재 브랜치 흐름은 `team`에서 공동 작업 후 PR로 `dev`에 병합하고, 검증된 `dev`만 PR로 `main`에 병합하는 방식이다.

| 브랜치 | 역할 | 운영 원칙 |
|------|------|------|
| `team` | 공동 구현 브랜치 | 작은 단위로 구현하고 테스트 결과를 공유 |
| `dev` | 팀 통합 브랜치 | PR 리뷰와 관련 테스트 확인 후 병합 |
| `main` | 최종 제출 브랜치 | 직접 push 금지, 검증된 `dev`만 반영 |
| 개인 실험 브랜치 | 디버깅 실험 | 최종 코드로 바로 병합하지 않고 근거만 공유 |

PR에는 관련 테스트, 수정 파일/함수, 구현 의도, 새 구조체 필드의 lifecycle, 회귀 가능성을 적는다. Driver가 아니었던 팀원 1명 이상이 리뷰 근거를 남긴다.

## 4. 공통 모듈 설계 원칙

10주차에는 구현 전 생성한 공통 skeleton이 많아지면서 설명하기 어려운 helper가 생겼다. 이번 주차에는 필요한 함수와 구조체를 먼저 팀이 정하고, 최소 단위로 추가한다.

- 공통 함수를 만들 때 호출자, 입력, 실패 반환값, cleanup 책임을 PR에 적는다.
- AI는 공통 모듈 초안 생성보다 설계 누락, 실패 케이스, 리뷰 체크리스트 확인에 사용한다.
- 테스트 하나를 통과시키기 위한 임시 helper는 임시 범위와 제거 계획을 남긴다.
- SPT entry, frame, swap slot, mmap file처럼 owner가 있는 자원은 누가 언제 해제하는지 설명할 수 있어야 한다.

## 5. 매일 공통 루틴

| 구간 | 할 일 | 산출물 |
|------|------|------|
| 시작 30분 | 전날 diff, 실패 테스트, panic log, `.output` 확인 | 오늘 첫 번째 실패 지점 확정 |
| 학습 60-90분 | GitBook과 `local/week11-12_study_docs`에서 오늘 구현 범위만 읽기 | 팀원이 답해야 할 질문 정리 |
| 구현 세션 | 작은 함수 단위로 작성하고 설계, cleanup, 테스트 조건 확인 | 빌드 가능한 작은 변경 |
| 테스트 루프 | 가까운 테스트 1-3개 실행 후 같은 묶음으로 확장 | 통과/실패 테스트와 재현 명령 기록 |
| 종료 30분 | 바뀐 구조체 필드, 자원 소유권, 남은 실패 원인 정리 | PR/WIL/이슈에 남길 문장 |

## 6. 일정과 현재 초점

공동 구현은 2026-05-11부터 2026-05-20까지 진행하고, 2026-05-21은 발표와 제출 정리에 집중한다. 2026-05-17은 공동 구현 제외일로 두고 mmap, dirty bit, eviction/swap을 개인 학습 범위로 삼았다.

현재 구현 초점은 아래 순서다.

1. SPT copy와 fork 기반 page lifecycle 정리
2. `mmap()` / `munmap()` 등록과 file-backed lazy read
3. dirty page write-back, frame eviction, anon/file-backed swap 통합
4. VM 전체 회귀와 Project 1/2, filesys/base 회귀 확인
5. 발표 자료와 WIL 정리

Extra COW는 VM 필수 범위가 안정된 뒤에만 판단한다.

## 7. 테스트 운영 원칙

테스트는 완료 여부 확인보다 설계 가설 검증 도구로 사용한다. 작은 변경 후 가까운 테스트 1-3개를 먼저 실행하고, 같은 묶음과 회귀 테스트로 넓힌다.

- `lazy-*` 실패: `load_segment()`, `lazy_load_segment()`, aux lifetime 확인
- `pt-*` 실패: address validation, stack growth 조건, writable bit 확인
- `page-*` 실패: lazy loading, SPT copy, fork, frame lifecycle 확인
- `mmap-*` 실패: syscall 인자 검증, overlap, fd/file lifetime, dirty write-back, `munmap()` cleanup 확인
- `swap-*` 실패: victim 선정, swap slot bitmap, anon/file-backed swap in/out 확인
- 회귀 실패: Project 2 process/fd lifecycle, Threads alarm/priority, filesys/base와 VM 변경의 상호작용 확인

실패 공유에는 테스트명, 재현 명령, 기대 결과, 실제 결과, 최근 수정 commit, 의심 함수, 다음 가설을 함께 남긴다.

## 8. 제출과 공유

최종 공유 발표는 2026-05-21 오전 10시이며, 팀 발표 자료와 노트북은 1개로 준비한다. 개인별 2분 발표 내용을 함께 준비하고, 2026-05-21 정오까지 주간 공유 발표 자료와 WIL을 제출한다.

WIL에는 단순 결과보다 배운 개념, 실패 원인, 수정 근거, 남은 리스크를 남긴다.
