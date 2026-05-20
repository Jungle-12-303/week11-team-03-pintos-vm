# 11-12주차 Pintos Virtual Memory 구현 계획

## 1. 문서 목적

이 문서는 Pintos `Project 3: Virtual Memory`의 현재 구현 기준선과 남은 구현 순서를 정리한다. 상세 날짜별 운영 기록은 `local/study_plan/week11-12/team-collaboration.md`를 기준으로 하며, 이 문서는 `docs/`에서 빠르게 볼 수 있는 실행 요약으로 유지한다.

협업 방식과 PR 기준은 [11-12주차 협업 문서](week11-12-collaboration.md)를 따른다.

## 2. 구현 범위

Project 3 필수 범위는 모두 구현 대상이다.

- SPT와 page lookup/insert/remove
- frame allocation, frame table, page claim
- ELF segment lazy loading
- page fault 처리와 stack growth
- SPT copy/kill과 fork/exit lifecycle
- `mmap()` / `munmap()`과 file-backed page
- dirty write-back
- page replacement와 anon/file-backed swap in/out
- Project 1/2, filesys/base 회귀 테스트

Extra 후보는 `cow-simple`로 제한하고, 필수 VM 범위가 안정된 뒤에만 판단한다.

## 3. 현재 구현 기준선

2026-05-15 오후 기준으로 `pair-a` 구현을 통합 기준선으로 선택했고, 이후 작업은 이 흐름 위에서 진행한다. 테스트 통과 여부는 별도 실행 결과로 기록하며, 이 문서는 구현 상태를 단정하지 않는다.

완료 또는 통합된 범위:

- SPT hash table 구조, hash callback, `supplemental_page_table_init()`
- `spt_find_page()`, `spt_insert_page()`, `spt_remove_page()` 기본 흐름
- `vm_alloc_page_with_initializer()` 기본 등록 흐름과 `struct page.writable`
- `vm_get_frame()`, `vm_claim_page()`, `vm_do_claim_page()` 기본 흐름
- VM용 첫 stack page claim
- `load_segment()`의 lazy page 등록과 `lazy_load_segment()`의 file read/zero fill
- `page_fault()`에서 `vm_try_handle_fault()`로 진입하는 흐름
- `anon_swap_in()`, `anon_swap_out()`, `anon_destroy()`를 위한 anon swap 상태 필드 초안
- `supplemental_page_table_kill()`, frame/page cleanup 경로
- `vm_stack_growth()`와 stack growth 후보 판단
- syscall 진입 시 user `rsp` 저장과 VM-aware user pointer validation 보완

부분 구현 또는 재점검 범위:

- `supplemental_page_table_copy()`의 uninit/anon 복제 흐름
- fork 기반 page lifecycle과 fd table 복제의 회귀 안정화
- frame table list/lock과 eviction skeleton
- `VM_FILE` 복제, mmap metadata, dirty write-back
- anon/file-backed swap 통합

미구현 또는 후속 구현 범위:

- `mmap()` / `munmap()` syscall 연결
- `do_mmap()`, `do_munmap()`
- `file_backed_initializer()`, `file_backed_swap_in()`, `file_backed_swap_out()`, `file_backed_destroy()`
- dirty page만 파일에 write-back하는 기준
- `vm_get_victim()`, `vm_evict_frame()` 실제 동작
- `swap-file`, `swap-fork`, `mmap-inherit`까지 포함한 전체 회귀

## 4. 남은 구현 순서

| 순서 | 목표 | 주요 파일 | 확인할 핵심 |
|------|------|------|------|
| 1 | SPT copy와 fork 회귀 정리 | `pintos/vm/vm.c`, `pintos/userprog/process.c` | uninit/anon page 복제, aux 복제/해제, fork 실패 cleanup |
| 2 | mmap 등록과 file-backed lazy read | `pintos/userprog/syscall.c`, `pintos/vm/file.c`, `pintos/include/vm/file.h` | mmap 인자 검증, overlap, `file_reopen()`, `VM_FILE` 등록, `file_backed_swap_in()` |
| 3 | 최소 `munmap()` cleanup | `pintos/vm/file.c`, `pintos/userprog/syscall.c` | mapping 범위 추적, SPT 제거, pml4 clear, frame/file cleanup |
| 4 | dirty write-back | `pintos/vm/file.c`, `pintos/vm/vm.c` | `pml4_is_dirty()`, `file_write_at()`, 마지막 page zero 영역 제외 |
| 5 | frame eviction과 swap 통합 | `pintos/vm/vm.c`, `pintos/vm/anon.c`, `pintos/vm/file.c` | victim selection, anon swap slot, file-backed swap-out/in |
| 6 | VM 전체 회귀와 제출 안정화 | 전체 영향 파일 | VM 필수, Project 2, Threads, filesys/base 결과 기록 |

구현 중 새 구조체 필드를 추가하면 생성 시점, 소유자, 해제 시점, 실패 시 cleanup 경로를 PR에 적는다.

## 5. 날짜별 초점

| 날짜 | 초점 | 산출물 |
|------|------|------|
| 2026-05-18 | mmap 등록, file-backed lazy read, eviction skeleton | `mmap()` 인자 검증, overlap 확인, `VM_FILE` lazy page 등록, clean page cleanup 가능한 `munmap()` 골격 |
| 2026-05-19 | dirty write-back, frame eviction, swap 통합 | dirty page write-back, `file_backed_swap_out()`, `vm_get_victim()`, `vm_evict_frame()`, anon/file-backed swap 분기 |
| 2026-05-20 | 전체 회귀와 제출 안정화 | VM/userprog/threads/filesys 테스트 결과, 실패 원인, rollback 필요한 임시 helper 기록 |
| 2026-05-21 | 발표와 제출 | 팀 발표 자료, 개인별 2분 발표 내용, WIL URL |

## 6. 모듈별 구현 기준

### SPT, fork, cleanup

- SPT key는 `pg_round_down(va)` 기준으로 유지한다.
- `supplemental_page_table_copy()`는 uninit, loaded anon, file-backed page를 구분한다.
- fork 실패 중간 경로에서 이미 복제된 page와 aux/file reference를 정리한다.
- `supplemental_page_table_kill()`은 process exit에서 남은 page/frame/file-backed 자원을 정리한다.

### mmap과 file-backed page

- `SYS_MMAP`, `SYS_MUNMAP`을 syscall dispatcher에 연결한다.
- `mmap()`은 `addr == NULL`, page misalignment, `length == 0`, fd 0/1, invalid fd, zero-length file, overlap, user range overflow를 실패 처리한다.
- `file_reopen()`으로 원본 fd close와 mmap lifetime을 분리한다.
- page별 file offset, read bytes, zero bytes, writable, mapping 범위를 추적한다.
- `file_backed_swap_in()`은 fault 시점에 파일에서 읽고 나머지를 zero fill한다.
- `do_munmap()`과 process exit은 dirty page만 write-back하고 SPT에서 page를 제거한다.

### eviction과 swap

- frame table에 user frame을 등록하고 victim selection 정책을 명확히 한다.
- eviction은 victim page의 `swap_out()`을 호출하고 pml4 mapping과 frame/page 연결을 정리한다.
- anon page는 swap disk slot을 사용한다.
- file-backed page는 backing file을 사용하며, dirty일 때만 write-back한다.
- swap in 성공 후 anon swap slot은 해제한다.

## 7. 테스트 운영 순서

테스트는 가까운 기능부터 좁게 확인한 뒤 확장한다.

1. 빌드와 smoke test
2. Project 2 기본 회귀: `args-*`, `halt`, `exit`, `read-*`, file syscall 일부
3. lazy loading: `lazy-file`, `lazy-anon`
4. stack/page fault: `pt-grow-stack`, `pt-grow-stk-sc`, `pt-grow-bad`, `pt-write-code*`, `pt-bad-*`
5. fork/page lifecycle: `fork-*`, `multi-*`, `page-linear`, `page-merge-*`, `page-shuffle`
6. mmap 기본: `mmap-read`, `mmap-close`, `mmap-unmap`
7. mmap 검증과 write-back: `mmap-bad-*`, `mmap-over-*`, `mmap-null`, `mmap-zero*`, `mmap-write`, `mmap-ro`, `mmap-clean`, `mmap-exit`
8. swap/eviction: `swap-anon`, `swap-file`, `swap-iter`, `swap-fork`
9. 전체 회귀: Project 2, Threads alarm/priority, filesys/base

## 8. 병합 게이트

`dev`에 올리는 PR은 최소한 아래 조건을 만족해야 한다.

- 관련 테스트와 재현 명령이 명시되어 있다.
- 수정한 파일과 핵심 함수가 정리되어 있다.
- 통과한 테스트와 남은 테스트가 구분되어 있다.
- page/frame/swap slot/file/aux 중 소유하거나 해제하는 자원이 설명되어 있다.
- invalid fault, write-protected fault, stack growth fault 중 어느 경로를 바꾸는지 설명되어 있다.
- mmap 변경은 fd/file lifetime, dirty write-back, `munmap()` cleanup 기준을 설명한다.
- eviction/swap 변경은 victim page, frame 재사용, swap slot 또는 backing file 책임을 설명한다.
- Project 2 회귀 가능성이 적혀 있다.
- Driver와 공동 검토자가 코드 흐름을 설명할 수 있다.

다음 경우에는 병합하지 않는다.

- 테스트만 통과하고 page lifecycle 설명이 없는 경우
- aux, file reference, swap slot, frame 해제 책임이 불명확한 경우
- fork 또는 process exit에서 누수가 의심되는데 확인하지 않은 경우
- Project 2 회귀 테스트가 깨졌는데 VM 테스트 통과만 근거로 올린 경우
- 임시 구현이 최종 구현처럼 설명된 경우

## 9. 완료 기준

- 4명이 SPT, lazy loading, stack growth, mmap, swap의 전체 흐름을 설명할 수 있다.
- VM 필수 테스트의 통과/실패 상태가 테스트 묶음별로 기록되어 있다.
- Project 2, Threads, filesys/base 회귀 범위를 확인했다.
- page/frame/swap/file-backed page의 소유권과 해제 경로가 코드와 PR 설명에 남아 있다.
- 발표 자료는 구현 현황, 주요 트러블슈팅, 남은 리스크, 개인별 회고를 포함한다.
- WIL은 단순 통과 목록이 아니라 실패 원인, 디버깅 근거, 구현 선택 이유를 포함한다.
