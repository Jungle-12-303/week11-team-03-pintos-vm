# 11-12주차 Pintos Virtual Memory 구현 기록

## 1. 문서 목적

이 문서는 Pintos `Project 3: Virtual Memory`의 2026-05-20 기준 구현 상태와 최종 정리 기준을 기록한다. 구현 세부 사항은 `pintos/` 실제 코드, KAIST Pintos GitBook 요구사항, [협업 문서](week11-12-collaboration.md)를 함께 확인한다.

협업 방식과 PR 기준은 [11-12주차 협업 문서](week11-12-collaboration.md)를 따른다.

## 2. 구현 범위

Project 3 필수 범위는 아래 항목을 기준으로 구현했다.

- SPT와 page lookup/insert/remove
- frame allocation, frame table, page claim
- ELF segment lazy loading
- page fault 처리와 stack growth
- SPT copy/kill과 fork/exit lifecycle
- `mmap()` / `munmap()`과 file-backed page
- dirty write-back
- page replacement와 anon/file-backed swap in/out
- Project 1/2, filesys/base 회귀 확인

Extra COW(`cow-simple`)는 필수 VM 구현과 분리된 선택 과제로 남겼다.

## 3. 현재 구현 기준선

2026-05-15에 `pair-a` 구현을 통합 기준선으로 선택했고, 이후 작업은 이 흐름 위에서 진행했다. 2026-05-20 기준으로 VM 필수 구현 범위와 발표 자료 정리를 마쳤다. 테스트 통과 여부, 재현 명령, 실패 로그는 PR, 이슈, WIL에 따로 기록한다.

완료 또는 통합된 범위:

- SPT hash table 구조, hash callback, `supplemental_page_table_init()`
- `spt_find_page()`, `spt_insert_page()`, `spt_remove_page()`
- `vm_alloc_page_with_initializer()`와 `struct page.writable`
- `vm_get_frame()`, `vm_claim_page()`, `vm_do_claim_page()`
- frame table list/lock, frame owner 추적, second-chance victim selection
- VM용 첫 stack page claim과 stack growth
- ELF lazy page 등록과 `lazy_load_segment()`의 file read/zero fill
- `page_fault()`에서 `vm_try_handle_fault()`로 진입하는 흐름
- syscall 진입 시 user `rsp` 저장과 VM-aware user pointer validation
- anon swap slot bitmap, `anon_swap_in()`, `anon_swap_out()`, `anon_destroy()`
- `supplemental_page_table_copy()`, `supplemental_page_table_kill()`
- fork에서 uninit anon page 즉시 claim, loaded anon page 복제, swap-out anon page 복제
- array-backed fd table, `fd_handle`, `process_duplicate_fds()` 기반 fork fd 복제
- `dup2()`로 같은 file offset을 공유하던 fd 관계를 fork 후 자식 안에서도 유지
- `child_status`, exec file write-deny, fd cleanup을 포함한 process lifecycle 정리
- `SYS_MMAP`, `SYS_MUNMAP`, `do_mmap()`, `do_munmap()`
- `file_backed_initializer()`, `file_backed_swap_in()`, `file_backed_swap_out()`, `file_backed_destroy()`
- mmap range metadata, `mmap_aux`, `file_page`, mmap 등록 실패 rollback
- dirty file-backed page write-back과 clean page skip
- unfaulted mmap page와 fault-in된 `VM_FILE` page의 fork 상속 제외
- `VM_ANON` lazy load aux와 `VM_FILE` mmap aux destroy 경로 분리

## 4. 날짜별 구현 기록

| 날짜 | 초점 | 산출물 |
|------|------|------|
| 2026-05-11 | SPT 기본 구조 | SPT hash table, `supplemental_page_table_init()`, `spt_find_page()`, `spt_insert_page()`, `spt_remove_page()` 구현 |
| 2026-05-12 | VM core와 첫 stack page | page allocation/claim 기본 흐름, `struct page.writable`, VM용 첫 stack page claim, uninit aux 해제 기본 경로 구현 |
| 2026-05-13 | ELF lazy loading과 page fault | lazy load aux, `load_segment()`, `lazy_load_segment()`, `page_fault()` -> `vm_try_handle_fault()` 연결 |
| 2026-05-14 | pair-a 통합 기준선 확정 | lazy loading/page fault 흐름을 pair-a 기준으로 통합하고, anon swap 상태 필드를 이후 eviction/fork 흐름에 반영할 기준으로 정리 |
| 2026-05-15 | SPT kill/cleanup과 stack growth | `supplemental_page_table_kill()`, `vm_dealloc_page()`, frame/page cleanup, stack growth 후보 판단, syscall user `rsp` 저장 |
| 2026-05-16 | SPT copy와 fork lifecycle | `process_fork()`, `__do_fork()`, `supplemental_page_table_copy()`, uninit/loaded anon page 복제, fd table 복제, frame table list/owner 구조 연결 |
| 2026-05-18 | mmap 등록과 file-backed lazy read | `SYS_MMAP`/`SYS_MUNMAP`, mmap 인자 검증, overlap 확인, `do_mmap()`, `lazy_load_file_page()`, mmap metadata 구현 |
| 2026-05-19 | munmap, dirty write-back, eviction/swap | `do_munmap()`, `file_backed_destroy()`, dirty bit 기반 write-back, `vm_get_victim()`, `vm_evict_frame()`, anon/file-backed swap in/out 연결 |
| 2026-05-20 | 보완과 발표 정리 | unfaulted mmap aux lifecycle, fork에서 mmap 상속 제외, `anon_copy_from_swap()` 기반 swap-out anon fork 복제, 팀 PPT와 개인 발표 자료 완성 |

## 5. 모듈별 구현 기준

### SPT와 frame

- SPT key는 `pg_round_down(va)` 기준으로 유지한다.
- page 생성은 `vm_alloc_page_with_initializer()`를 통해 SPT에 `VM_UNINIT` page를 등록하는 흐름을 따른다.
- page claim은 `vm_claim_page()`에서 SPT page를 찾고 `vm_do_claim_page()`에서 frame, page, pml4 mapping을 연결한다.
- frame table은 user frame을 관리하며, `vm_get_victim()`은 accessed bit를 이용한 second-chance 방식으로 victim을 고른다.
- eviction은 victim page의 `swap_out()`을 호출하고 pml4 mapping, frame/page 연결, owner를 정리한 뒤 frame을 재사용한다.

### Lazy loading과 page fault

- ELF segment는 `load_segment()`에서 eager load하지 않고 lazy page로 등록한다.
- 실제 파일 read와 zero fill은 최초 page fault 시 `lazy_load_segment()`에서 수행한다.
- `vm_try_handle_fault()`는 invalid address, write-protected fault, SPT lookup, stack growth 후보 여부를 분기한다.
- syscall 경로에서는 user pointer validation 중 lazy claim과 stack growth가 필요할 수 있으므로 syscall 진입 당시 user `rsp`를 저장한다.

### Fork와 SPT copy

- `process_fork()`는 부모의 `intr_frame`과 `child_status`를 aux에 담고 자식 초기화 결과를 `load_sema`로 기다린다.
- `__do_fork()`는 자식 pml4를 만든 뒤 `supplemental_page_table_copy()`로 부모 SPT를 복제하고 fd table을 복제한다.
- `VM_UNINIT + VM_ANON` page는 lazy metadata를 복제한 뒤 즉시 `vm_claim_page()`한다.
- loaded `VM_ANON` page는 자식 anon page와 frame을 새로 만들고 부모 frame 내용을 복사한다.
- 부모 anon page가 swap-out되어 `page->frame == NULL`인 경우에는 부모 page를 swap-in하지 않고 `anon_copy_from_swap()`으로 부모 swap slot을 읽어 자식 frame에 복사한다.
- mmap file-backed page는 현재 테스트 정책에 맞춰 fork에서 상속하지 않는다. `VM_UNINIT + VM_FILE` page와 이미 fault-in된 `VM_FILE` page를 모두 건너뛴다.

### Project 2 process/fd lifecycle 연계

- fd table은 array-backed table로 관리하고, fd entry는 reference-counted `fd_handle`을 가리킨다.
- `dup2()`로 같은 handle을 공유하던 fd들은 같은 file offset을 공유해야 하므로, fork 시 source handle과 destination handle의 대응 관계를 유지한다.
- 일반 file fd와 mmap page의 독립 file reference는 `file_duplicate()`로 만든다.
- `process_duplicate_fds()`는 부모 fd table을 자식에게 복제하고, 중간 실패 시 이미 만든 child-side fd handle과 fd entry를 정리한다.
- `child_status`는 fork/load 성공 여부, wait 한 번만 허용하는 상태, exit status, 부모/자식 참조 수를 관리한다.
- `process_close_exec_file()`은 exec file write-deny를 해제하고 파일을 닫으며, `process_release_children()`과 `process_close_all_files()`는 process exit 경로에서 남은 parent-side child 상태와 fd를 정리한다.

### mmap과 file-backed page

- `SYS_MMAP`, `SYS_MUNMAP`은 syscall dispatcher에서 `do_mmap()`, `do_munmap()`으로 연결한다.
- `mmap()`은 `addr == NULL`, page misalignment, invalid offset, `length == 0`, invalid fd, zero-length file, overlap, user range overflow를 실패 처리한다.
- 각 mmap page는 fault 전에는 `struct mmap_aux`, fault 후에는 `struct file_page`로 file, offset, read bytes, zero bytes, map base, page count를 관리한다.
- 각 mmap page는 cleanup 전까지 독립적인 file reference를 가진다. 현재 구현은 `file_duplicate()`를 사용해 원본 fd close와 mmap lifetime을 분리한다.
- `file_backed_swap_in()`과 `lazy_load_file_page()`는 file에서 필요한 바이트를 읽고 나머지를 zero fill한다.
- `do_munmap()`은 mmap 시작 주소에서 page count를 확인하고, 같은 mapping에 속한 page를 SPT에서 제거한다.
- fault되지 않은 mmap page는 `mmap_aux_destroy()`로 file reference를 닫고, fault-in된 page는 `file_backed_destroy()`에서 dirty write-back과 frame/file cleanup을 수행한다.
- dirty write-back은 `pml4_is_dirty()`를 기준으로 `read_bytes`만 `file_write_at()`으로 기록하고 dirty bit를 내린다.

### anon swap

- anon page는 swap disk slot과 `in_swapdisk` 상태를 가진다.
- `anon_swap_out()`은 빈 swap slot을 확보하고 frame 내용을 disk sector에 기록한다.
- `anon_swap_in()`은 swap slot에서 frame으로 복원한 뒤 bitmap slot을 해제한다.
- `anon_destroy()`는 page가 swap slot을 소유하고 있으면 bitmap을 해제하고, frame이 있으면 frame cleanup을 수행한다.
- `anon_copy_from_swap()`은 fork 전용 보조 흐름으로, 부모 swap slot을 해제하지 않고 읽기만 해서 자식 frame에 복사한다.

## 6. 테스트 운영 순서

테스트는 가까운 기능부터 좁게 확인한 뒤 확장한다. 결과 자체는 PR, 이슈, WIL에 기록한다.

1. 빌드와 smoke test
2. Project 2 기본 회귀: `args-*`, `halt`, `exit`, `read-*`, file syscall 일부
3. lazy loading: `lazy-file`, `lazy-anon`
4. stack/page fault: `pt-grow-stack`, `pt-grow-stk-sc`, `pt-grow-bad`, `pt-write-code*`, `pt-bad-*`
5. fork/page lifecycle: `fork-*`, `multi-*`, `page-linear`, `page-merge-*`, `page-shuffle`
6. mmap 기본: `mmap-read`, `mmap-close`, `mmap-unmap`
7. mmap 검증과 write-back: `mmap-bad-*`, `mmap-over-*`, `mmap-null`, `mmap-zero*`, `mmap-write`, `mmap-ro`, `mmap-clean`, `mmap-exit`, `mmap-inherit`
8. swap/eviction: `swap-anon`, `swap-file`, `swap-iter`, `swap-fork`
9. 전체 회귀: Project 2, Threads alarm/priority, filesys/base

## 7. 병합 게이트

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

## 8. 완료 기준

- 4명이 SPT, lazy loading, stack growth, mmap, swap의 전체 흐름을 설명할 수 있다.
- VM 필수 테스트의 통과/실패 상태가 테스트 묶음별로 기록되어 있다.
- Project 2, Threads, filesys/base 회귀 범위를 확인했다.
- page/frame/swap/file-backed page의 소유권과 해제 경로가 코드와 PR 설명에 남아 있다.
- 발표 자료는 구현 현황, 주요 트러블슈팅, 남은 리스크, 개인별 회고를 포함한다.
- WIL은 단순 통과 목록이 아니라 실패 원인, 디버깅 근거, 구현 선택 이유를 포함한다.
