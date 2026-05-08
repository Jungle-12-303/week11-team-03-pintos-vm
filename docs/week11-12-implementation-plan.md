# 11-12주차 Pintos Virtual Memory 구현 계획

## 1. 문서 목적

이 문서는 11-12주차 Pintos `Project 3: Virtual Memory`의 2주 구현 순서와 테스트 운영 기준을 정리한 실행 문서다. 협업 방식과 PR 기준은 [11-12주차 협업 문서](week11-12-collaboration.md)에서 관리하고, 이 문서는 무엇을 어떤 순서로 구현하고 어떤 테스트로 확인할지에 집중한다.

이번 주차는 새 팀원 4명이 함께 진행하므로, 작업을 4개로 크게 쪼개 장기 병렬 구현하지 않는다. 핵심 VM 설계와 실제 구현은 4명이 같은 세션에서 함께 진행한다. 명칭은 페어 프로그래밍을 사용하지만, 운영 방식은 한 명이 코드를 작성하고 나머지 세 명이 설계, 검증, 기록을 동시에 맡는 4인 공동 코딩이다.

## 2. 구현 범위

공지 기준 `Project 3: Virtual Memory`는 모두 필수 구현 사항이다. Extra는 필수 VM 구현을 안정화한 뒤 남은 시간이 있을 때만 진행한다.

이번 구현 계획의 필수 범위는 다음과 같다.

- 보조 페이지 테이블(SPT)
- page fault 처리
- lazy loading
- frame table과 frame claiming
- page replacement
- stack growth
- memory mapped files
- anonymous/file-backed page swap in/out
- process fork 시 SPT 복제
- process exit 시 SPT, frame, swap slot, mmap write-back 정리
- Project 2/User Programs와 Project 1/Threads 회귀 테스트 확인

Extra 후보는 `cow-simple` 하나로 제한한다.

## 3. 학습 키워드

- Virtual Memory
- Supplemental Page Table
- Page Fault
- Lazy Loading
- Frame Table
- Page Replacement
- Swap In/Out
- Stack Growth
- Memory Mapped File
- Dirty Bit
- Accessed Bit
- Copy-on-Write

## 4. 코드 기준선

현재 저장소는 Project 2 구현 흔적을 포함하고 있으며, VM 구현은 골격과 TODO가 많이 남아 있다. 11-12주차 첫 작업은 기존 코드를 유지하면서 VM 분기에서 필요한 빈칸을 채우는 것이다.

주요 수정 후보 파일은 다음과 같다.

| 파일 | 역할 |
|------|------|
| `pintos/vm/vm.c` | SPT, frame allocation, page claim, page fault, stack growth, eviction |
| `pintos/vm/anon.c` | anonymous page 초기화와 swap in/out |
| `pintos/vm/file.c` | file-backed page, mmap, munmap, write-back |
| `pintos/vm/uninit.c` | lazy page 초기화와 aux 정리 |
| `pintos/include/vm/vm.h` | `struct page`, `struct frame`, `struct supplemental_page_table` 확장 |
| `pintos/include/vm/anon.h` | anonymous page 상태 확장 |
| `pintos/include/vm/file.h` | file-backed page와 mmap 상태 확장 |
| `pintos/userprog/exception.c` | page fault handler와 VM fault 처리 연결 |
| `pintos/userprog/process.c` | `load_segment`, `lazy_load_segment`, `setup_stack`, SPT copy/kill 연동 |
| `pintos/userprog/syscall.c` | `mmap`, `munmap`, VM 환경의 user pointer 검증 보완 |
| `pintos/include/threads/thread.h` | thread별 SPT와 필요 시 mmap/fd 관련 상태 보관 |

`pintos/filesys/*` 내부 구현은 Project 3 요구와 직접 관련이 있을 때만 수정한다. mmap과 file-backed page는 우선 VM/userprog 계층에서 해결한다.

## 5. 2주 운영 방식

2주 동안 매일 같은 리듬으로 운영한다.

1. 오전 또는 작업 시작 직후 30분: 전날 코드와 실패 테스트를 4명이 함께 읽는다.
2. 첫 번째 구현 세션: 4명이 같은 화면에서 하나의 구현 목표를 진행한다.
3. 중간 점검: Driver가 바꾼 함수와 새 상태를 설명하고, Navigator/Reviewer/Recorder가 설계 근거와 테스트 조건을 확인한다.
4. 두 번째 구현 세션: Driver를 교체하고 같은 작업 흐름을 이어 간다.
5. 마감 전 30분: 통과/실패 테스트, 다음 fault 지점, 발표/WIL에 남길 트러블슈팅을 기록한다.

역할은 고정하지 않고 30-60분 단위로 교체한다.

| 역할 | 당일 책임 |
|------|------|
| Driver | 키보드를 잡고 작은 단위로 구현 |
| Navigator | GitBook, 현재 코드, 테스트 조건을 보며 설계 확인 |
| Reviewer | page/frame/swap/file 자원 소유권과 cleanup 경로 확인 |
| Recorder | 결정 사항, 테스트 명령, 실패 로그, 다음 확인 항목 기록 |

다음 작업 묶음으로 넘어갈 때도 4명이 함께 이동한다. 병렬 구현은 하지 않고, 필요한 경우 한 명이 조사한 내용을 즉시 세션에 공유한 뒤 코드 반영 여부를 전원이 결정한다.

## 6. 전체 구현 순서

구현은 아래 순서를 따른다.

1. 기준선 확인과 Project 2 회귀 상태 점검
2. SPT 자료구조와 page lookup/insert/remove
3. frame allocation과 page claim
4. ELF segment lazy loading
5. VM용 `setup_stack()`과 기본 page fault 처리
6. SPT copy/kill과 fork 회귀 복구
7. stack growth
8. mmap/munmap과 file-backed page
9. page replacement와 anonymous page swap
10. file-backed page swap/write-back 정리
11. VM 전체 테스트와 Project 1/2 회귀 테스트
12. Extra COW 판단 또는 발표/WIL 정리

이 순서를 유지하는 이유는 다음과 같다.

- SPT가 없으면 page fault에서 유효한 fault를 복구할 근거가 없다.
- frame claim이 없으면 lazy page를 실제 물리 프레임에 연결할 수 없다.
- lazy loading이 안정되어야 Project 2 user program 실행 경로를 VM 위에서 다시 살릴 수 있다.
- fork는 SPT copy와 fd table 복제가 함께 맞아야 `page-*`, `mmap-inherit`, `swap-fork` 디버깅이 가능하다.
- stack growth는 page fault 처리와 SPT insertion이 안정된 뒤에 붙이는 편이 안전하다.
- mmap은 file-backed page와 dirty write-back 책임이 필요하므로 lazy loading 이후에 구현한다.
- swap과 eviction은 모든 page type의 lifecycle을 건드리므로 후반에 통합한다.

## 7. 1주차 계획

1주차 목표는 "VM 기반 사용자 프로그램 실행 경로를 살리고, lazy loading과 stack growth의 핵심 테스트를 통과할 수 있는 상태"를 만드는 것이다.

| 시점 | 공통 목표 | 주요 파일 | 확인 테스트 |
|------|------|------|------|
| 1일차 | 공지, GitBook, 현재 코드 기준선 읽기 | `README.md`, `docs/gitbook/한국어/04_project3_virtual_memory/*`, `pintos/vm/*` | 빌드 가능 여부, 현재 실패 기준 |
| 1-2일차 | SPT 구조 결정과 기본 함수 구현 | `include/vm/vm.h`, `vm/vm.c` | VM 빌드, `spt_find_page` 단위 흐름 코드 리뷰 |
| 2-3일차 | frame allocation과 `vm_claim_page()` 구현 | `vm/vm.c` | page claim 흐름, 기본 userprog 실행 관찰 |
| 3-4일차 | lazy loading과 VM용 stack setup 구현 | `userprog/process.c`, `vm/uninit.c`, `vm/vm.c` | `lazy-file`, `lazy-anon`, Project 2 기본 실행 |
| 4-5일차 | `vm_try_handle_fault()`와 Project 2 회귀 보완 | `userprog/exception.c`, `vm/vm.c`, `userprog/syscall.c` | `args-*`, `halt`, `exit`, file syscall 일부 |
| 5-6일차 | SPT copy/kill, fork 회귀 복구 | `vm/vm.c`, `userprog/process.c` | `fork-*`, `multi-*`, `page-linear` |
| 6-7일차 | stack growth 구현 | `vm/vm.c` | `pt-grow-stack`, `pt-grow-stk-sc`, `pt-big-stk-obj`, `pt-grow-bad` |

1주차 말에는 아래 내용을 팀원 4명이 설명할 수 있어야 한다.

- SPT entry가 어떤 key로 저장되는지
- uninit page가 page fault 시 어떤 순서로 anon/file page로 변하는지
- `lazy_load_segment()`에서 file offset, read bytes, zero bytes가 어떻게 쓰이는지
- `setup_stack()`에서 첫 stack page를 즉시 claim하는 이유
- fork 시 부모 SPT를 child SPT로 복제하는 기준
- process exit 시 SPT에 남아 있는 uninit/page/frame 자원을 정리하는 흐름

## 8. 2주차 계획

2주차 목표는 "mmap, swap, page replacement까지 포함해 VM 필수 테스트를 안정화하고 회귀 테스트를 닫는 것"이다.

| 시점 | 공통 목표 | 주요 파일 | 확인 테스트 |
|------|------|------|------|
| 8일차 | mmap/munmap syscall 연결 | `userprog/syscall.c`, `vm/file.c`, `include/vm/file.h` | `mmap-read`, `mmap-close`, `mmap-unmap` |
| 9일차 | file-backed page lazy load와 write-back | `vm/file.c`, `vm/vm.c` | `mmap-write`, `mmap-ro`, `mmap-clean`, `mmap-exit` |
| 10일차 | mmap invalid case와 overlap 검증 | `vm/file.c`, `userprog/syscall.c` | `mmap-bad-*`, `mmap-over-*`, `mmap-null`, `mmap-zero*` |
| 11일차 | frame table과 victim selection | `vm/vm.c`, `include/vm/vm.h` | `page-parallel`, `page-shuffle`, `page-merge-*` |
| 12일차 | anonymous page swap in/out | `vm/anon.c`, `vm/vm.c`, `include/vm/anon.h` | `swap-anon`, `swap-iter` |
| 13일차 | file-backed swap/write-back와 fork 연계 | `vm/file.c`, `vm/vm.c`, `userprog/process.c` | `swap-file`, `swap-fork`, `mmap-inherit`, `mmap-shuffle` |
| 14일차 | 전체 회귀, 발표/WIL 정리, Extra 판단 | 전체 영향 파일 | VM 전체, userprog, threads, filesys/base |

2주차 말에는 아래 조건을 완료 기준으로 삼는다.

- VM 필수 테스트를 묶음별로 실행하고 결과를 기록한다.
- Project 2 회귀 테스트 중 CSV에 포함된 userprog 테스트를 확인한다.
- Threads alarm/priority 회귀 테스트를 확인한다.
- filesys/base 회귀 테스트를 확인한다.
- 실패 테스트가 남으면 원인, 재현 명령, 관련 함수, 다음 조치가 문서 또는 이슈에 남아 있다.

## 9. 구현 단계별 세부 계획

### 9.1 SPT

목표는 virtual address에서 `struct page`를 안정적으로 찾는 것이다.

주요 작업은 다음과 같다.

- `struct supplemental_page_table`에 해시 또는 리스트 기반 자료구조를 둔다.
- page key는 `pg_round_down(va)`로 정규화한다.
- `supplemental_page_table_init()`에서 자료구조를 초기화한다.
- `spt_find_page()`는 정렬된 user page address로 page를 찾는다.
- `spt_insert_page()`는 중복 page를 거부한다.
- `spt_remove_page()`와 `supplemental_page_table_kill()`은 page destroy 경로와 연결한다.

병합 전 확인 포인트는 중복 insert, uninit page cleanup, process exit cleanup이다.

### 9.2 Frame Claim

목표는 SPT에 등록된 page를 실제 frame에 연결하고 page table mapping을 만드는 것이다.

주요 작업은 다음과 같다.

- `vm_get_frame()`에서 `PAL_USER`로 user frame을 할당한다.
- frame table을 도입해 frame과 page의 양방향 참조를 관리한다.
- `vm_claim_page()`는 SPT에서 page를 찾고 `vm_do_claim_page()`를 호출한다.
- `vm_do_claim_page()`는 frame 연결, `pml4_set_page()`, `swap_in()`을 책임진다.
- 실패 시 frame/page 상태가 반쯤 연결된 채 남지 않게 정리한다.

초기에는 eviction 없이 free frame만 사용해도 된다. 단, 코드 구조는 후속 eviction을 붙일 수 있게 둔다.

### 9.3 Lazy Loading

목표는 ELF segment를 load 시점에 전부 읽지 않고, page fault 시점에 필요한 page만 읽는 것이다.

주요 작업은 다음과 같다.

- `load_segment()`에서 page별 aux를 만들고 `vm_alloc_page_with_initializer()`로 등록한다.
- aux에는 file, offset, read bytes, zero bytes, writable 정보를 담는다.
- `lazy_load_segment()`는 file seek/read, zero fill, aux 해제를 책임진다.
- lazy loading 동안 실행 파일이 닫히거나 offset이 깨지지 않도록 file ownership을 명확히 한다.
- `uninit_destroy()`에서 fault되지 않은 page의 aux가 누수되지 않게 한다.

확인 테스트는 `lazy-file`, `lazy-anon`, Project 2의 `args-*`, `halt`, `exit`부터 시작한다.

### 9.4 Page Fault

목표는 복구 가능한 fault는 page를 claim하고, 잘못된 접근은 process를 종료하는 것이다.

주요 작업은 다음과 같다.

- `page_fault()`가 `vm_try_handle_fault()` 성공 시 반환하게 둔다.
- `vm_try_handle_fault()`에서 null, kernel address, not-present, write permission을 검증한다.
- SPT에 등록된 page면 `vm_do_claim_page()`로 복구한다.
- writable이 아닌 page에 write하는 경우는 실패 처리한다.
- stack growth 후보는 별도 기준으로 판단해 `vm_stack_growth()`로 보낸다.

확인 테스트는 `pt-bad-addr`, `pt-bad-read`, `pt-write-code`, `pt-write-code2`다.

### 9.5 SPT Copy와 Kill

목표는 fork와 process exit에서 VM 자원을 일관되게 복제하고 해제하는 것이다.

주요 작업은 다음과 같다.

- `supplemental_page_table_copy()`에서 uninit page와 loaded page를 구분해 복제한다.
- loaded anon page는 child frame에 내용을 복사한다.
- file-backed page는 mmap semantics에 맞게 file 정보와 offset을 복제한다.
- `supplemental_page_table_kill()`은 모든 page를 destroy하고 frame mapping을 끊는다.
- dirty mmap page는 종료 시 write-back한다.

확인 테스트는 `fork-*`, `multi-*`, `page-linear`, `mmap-inherit`, `swap-fork`다.

### 9.6 Stack Growth

목표는 정상적인 stack 접근은 확장하고, 비정상 주소 접근은 종료하는 것이다.

주요 작업은 다음과 같다.

- `fault_addr`가 user address인지 확인한다.
- fault address가 현재 rsp 근처인지 확인한다.
- 최대 stack 크기 제한을 둔다.
- `vm_stack_growth()`는 page-aligned address부터 필요한 page를 할당하고 claim한다.
- stack marker가 필요하면 `VM_MARKER_0` 같은 marker를 사용한다.

확인 테스트는 `pt-grow-stack`, `pt-grow-stk-sc`, `pt-big-stk-obj`, `pt-grow-bad`다.

### 9.7 mmap/munmap

목표는 file을 user virtual address 범위에 매핑하고, munmap/exit 시 변경 내용을 파일에 반영하는 것이다.

주요 작업은 다음과 같다.

- `SYS_MMAP`, `SYS_MUNMAP`을 syscall dispatcher에 연결한다.
- mmap 인자 검증을 구현한다.
- 주소는 page-aligned여야 하며, length 0, fd 0/1, invalid fd, overlap은 실패 처리한다.
- file page별 aux를 구성해 `VM_FILE` page로 SPT에 등록한다.
- `do_munmap()`은 매핑 범위의 page를 순회하며 dirty page를 write-back하고 SPT에서 제거한다.
- mmap 중인 file은 원본 fd close와 독립적으로 유지되도록 file duplicate 또는 reopen 정책을 명확히 한다.

확인 테스트는 `mmap-read`, `mmap-close`, `mmap-unmap`, `mmap-write`, `mmap-ro`, `mmap-bad-*`, `mmap-over-*`다.

### 9.8 Eviction과 Swap

목표는 user pool이 부족할 때 frame을 축출하고, 필요한 page를 다시 복구하는 것이다.

주요 작업은 다음과 같다.

- frame table에 모든 user frame을 등록한다.
- victim selection 정책을 정한다. 초기 정책은 단순 clock 또는 FIFO로 시작한다.
- `vm_evict_frame()`은 victim page를 swap out하고 page table mapping을 제거한다.
- anon page는 swap disk slot에 기록한다.
- file-backed page는 dirty 여부와 mapping 종류에 따라 파일 write-back 또는 drop을 결정한다.
- swap slot bitmap을 두고 slot 할당과 해제를 추적한다.
- swap in 성공 후 slot을 해제한다.

확인 테스트는 `page-shuffle`, `page-merge-*`, `swap-anon`, `swap-file`, `swap-iter`, `swap-fork`다.

## 10. 테스트 운영 순서

테스트는 아래 순서로 좁게 확인한 뒤 넓힌다.

1. 빌드 확인
   - `pintos/vm`에서 kernel과 user test binary가 컴파일되는지 확인한다.

2. Project 2 기본 회귀
   - `args-*`
   - `halt`
   - `exit`
   - file syscall 일부

3. lazy loading
   - `lazy-file`
   - `lazy-anon`

4. page fault와 stack growth
   - `pt-grow-stack`
   - `pt-grow-stk-sc`
   - `pt-big-stk-obj`
   - `pt-grow-bad`
   - `pt-bad-addr`
   - `pt-bad-read`
   - `pt-write-code`
   - `pt-write-code2`

5. process/fork 기반 page 테스트
   - `page-linear`
   - `page-parallel`
   - `page-merge-seq`
   - `page-merge-par`
   - `page-merge-stk`
   - `page-merge-mm`
   - `page-shuffle`

6. mmap
   - `mmap-read`
   - `mmap-close`
   - `mmap-unmap`
   - `mmap-write`
   - `mmap-ro`
   - `mmap-exit`
   - `mmap-shuffle`
   - `mmap-*` invalid case

7. swap
   - `swap-file`
   - `swap-anon`
   - `swap-iter`
   - `swap-fork`

8. 전체 회귀
   - CSV에 포함된 Project 2 테스트
   - CSV에 포함된 Threads alarm/priority 테스트
   - filesys/base 테스트

9. Extra 판단
   - `cow-simple`

## 11. 병합 게이트

`dev`에 올리는 PR은 최소한 아래 조건을 만족해야 한다.

- 관련 테스트 이름이 명시되어 있다.
- 수정한 파일과 핵심 함수가 정리되어 있다.
- 통과한 테스트와 남은 테스트가 구분되어 있다.
- page/frame/swap/file aux 중 소유하거나 해제하는 자원이 설명되어 있다.
- invalid fault, write-protected fault, stack growth fault 중 어느 경로를 바꾸는지 설명되어 있다.
- Project 2 회귀 가능성이 적혀 있다.
- Driver와 공동 검토자가 코드 흐름을 설명할 수 있다.
- Driver가 아니었던 팀원 1명 이상이 PR에서 리뷰 근거를 남긴다.
- 결합도가 높은 파일 변경은 4명이 함께 확인한다.

다음 경우에는 병합하지 않는다.

- 테스트만 통과하고 page lifecycle 설명이 없는 경우
- aux, file duplicate, swap slot, frame 해제 책임이 불명확한 경우
- fork 또는 process exit에서 누수가 의심되는데 확인하지 않은 경우
- Project 2 회귀 테스트가 깨졌는데 VM 테스트 통과만 근거로 올린 경우
- 임시 구현이 최종 구현처럼 설명된 경우

## 12. 충돌 가능 파일

아래 파일은 여러 작업 묶음이 겹칠 가능성이 높으므로 병렬 수정 시 특히 주의한다.

- `pintos/vm/vm.c`
- `pintos/vm/anon.c`
- `pintos/vm/file.c`
- `pintos/vm/uninit.c`
- `pintos/include/vm/vm.h`
- `pintos/include/vm/anon.h`
- `pintos/include/vm/file.h`
- `pintos/include/vm/uninit.h`
- `pintos/userprog/process.c`
- `pintos/userprog/exception.c`
- `pintos/userprog/syscall.c`
- `pintos/include/threads/thread.h`
- `pintos/include/userprog/process.h`
- `pintos/include/userprog/syscall.h`

위 파일은 개인이 단독으로 오래 들고 가지 않는다. 필요한 변경은 작은 PR로 만들고 4인 공동 리뷰를 거친다.

## 13. 일일 점검 항목

매일 작업 종료 전에 아래를 확인한다.

- 오늘 바뀐 구조체 필드와 그 lifecycle을 4명이 이해했는가
- 새로 통과한 테스트와 새로 깨진 테스트는 무엇인가
- 다음에 볼 실패 테스트의 첫 번째 fault 지점은 어디인가
- `process_exit()` 또는 `supplemental_page_table_kill()`에서 해제되지 않는 자원이 있는가
- mmap 또는 swap 관련 file/slot/frame 소유권이 모호하지 않은가
- 발표와 WIL에 남길 트러블슈팅 근거가 기록되었는가

## 14. 완료 기준

11-12주차 완료 기준은 다음과 같다.

- 4명이 SPT, lazy loading, stack growth, mmap, swap의 전체 흐름을 설명할 수 있다.
- VM 필수 테스트의 통과/실패 상태가 테스트 묶음별로 기록되어 있다.
- Project 2, Threads, filesys/base 회귀 범위를 확인했다.
- page/frame/swap/file-backed page의 소유권과 해제 경로가 코드와 PR 설명에 남아 있다.
- 발표 자료는 구현 현황, 주요 트러블슈팅, 남은 리스크, 개인별 회고를 포함한다.
- WIL은 단순 통과 목록이 아니라 실패 원인, 디버깅 근거, 구현 선택 이유를 포함한다.
- Extra COW는 필수 VM 구현이 안정된 뒤에만 진행한다.
