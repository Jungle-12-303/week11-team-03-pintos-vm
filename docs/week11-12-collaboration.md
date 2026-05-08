# 11-12주차 Pintos Virtual Memory 협업 문서

## 1. 문서 목적

이 문서는 11-12주차 Pintos `Project 3: Virtual Memory`를 4인 팀이 함께 진행할 때의 협업 기준을 정리한 공식 문서다. 이번 주차에는 새롭게 팀원이 변경되었으므로, 기존 9주차와 10주차 문서는 참고용으로만 사용하고 11-12주차 운영 기준은 이 문서를 따른다.

구체적인 구현 순서와 테스트 운영 기준은 [11-12주차 구현 계획 문서](week11-12-implementation-plan.md)에서 별도로 관리한다.

## 2. 이번 주차 목표

공지 기준으로 11-12주차의 중심 과제는 `Project 3: Virtual Memory`다. Project 3은 모두 필수 구현 사항이며, Extra는 Virtual Memory를 일찍 끝낸 뒤 선택적으로 진행한다.

이번 주차의 목표는 단순히 VM 테스트를 통과하는 것이 아니라, 팀원 4명이 아래 흐름을 공통 언어로 설명할 수 있게 되는 것이다.

- 보조 페이지 테이블(SPT)이 페이지 폴트 처리의 기준 데이터가 되는 이유
- lazy loading이 ELF segment 적재 방식을 바꾸는 지점
- page fault handler가 유효한 fault와 종료해야 할 fault를 구분하는 기준
- frame table과 page replacement가 필요한 이유
- stack growth가 허용되는 주소와 거부되는 주소의 차이
- memory mapped file이 file descriptor, file object, file-backed page와 연결되는 방식
- anonymous page와 file-backed page의 swap in/out 차이
- Project 2의 process, syscall, file descriptor 구현이 VM 위에서 회귀 없이 동작해야 하는 이유

## 3. 협업 방식

11-12주차는 작업을 4명에게 크게 쪼개서 각자 독립 구현하는 방식보다, 공통 이해와 페어 프로그래밍을 우선한다. 여기서 말하는 페어 프로그래밍은 2명 단위 분업이 아니라, 4명이 동시에 같은 코드와 테스트 로그를 보며 한 명이 작성하고 세 명이 설계, 검증, 기록을 함께 맡는 공동 코딩 방식이다. Virtual Memory는 `process.c`, `exception.c`, `vm.c`, `anon.c`, `file.c`, `thread.h`가 서로 강하게 연결되어 있어 모듈별 장기 분업을 하면 병합 시 설계가 쉽게 어긋난다.

기본 운영 방식은 다음과 같다.

- 핵심 설계 결정은 4명이 함께 정한다.
- 실제 구현은 4명이 같은 세션에서 진행한다.
- 한 명이 Driver로 코드를 작성하고, 나머지 세 명은 Navigator, Reviewer, Recorder 역할을 맡아 설계 근거와 테스트 조건을 동시에 확인한다.
- Driver는 30-60분 단위로 교체해 같은 사람이 계속 같은 영역만 작성하지 않게 한다.
- `vm.c`, `process.c`, `syscall.c`, `exception.c`, `thread.h`처럼 결합도가 높은 파일은 병합 전 4명이 함께 흐름을 확인한다.
- 구현이 늦어지더라도 "한 명만 아는 코드"가 `dev` 또는 최종 브랜치에 들어가지 않게 한다.

## 4. 페어 프로그래밍 규칙

명칭은 페어 프로그래밍을 사용하지만, 실제 운영 단위는 4인 공동 세션이다. 한 세션에서는 Driver 1명과 공동 검토자 3명을 둔다.

- Driver는 키보드를 잡고 작은 단위로 구현한다.
- Navigator는 GitBook, 현재 코드, 테스트 기대 동작을 보며 설계가 벗어나지 않는지 확인한다.
- Reviewer는 방금 작성한 코드의 자원 소유권, cleanup 경로, 회귀 가능성을 확인한다.
- Recorder는 결정한 설계, 테스트 명령, 실패 로그, 다음 확인 항목을 이슈나 PR 설명에 남긴다.
- 역할은 30-60분 단위로 교체한다.
- 세션이 막히면 구현을 멈추고 4명이 fault 재현, call stack, 관련 구조체 상태를 함께 다시 본다.
- 세션이 끝나면 구현한 함수, 바꾼 상태, 깨질 수 있는 테스트를 5분 안에 정리한다.

페어 프로그래밍의 목적은 속도보다 공유된 이해다. 특히 VM에서는 `struct page`, `struct frame`, `supplemental_page_table`, `struct thread`에 추가한 상태가 여러 경로에서 해제되므로, Driver가 아니었던 팀원도 lifecycle을 설명할 수 있어야 한다.

## 5. 브랜치와 병합 원칙

팀 최종 결과물은 하나의 코드베이스로 관리한다. 공지에서는 팀 repository의 `master` branch를 최종 결과물로 언급하지만, 실제 팀 repository의 기본 브랜치가 `main`이면 `main`을 최종 브랜치로 사용한다. 팀은 첫날에 최종 브랜치 이름을 하나로 확정한다.

권장 브랜치 흐름은 다음과 같다.

| 브랜치 | 역할 | 운영 원칙 |
|------|------|------|
| `main` 또는 `master` | 최종 제출 기준 브랜치 | 직접 push 금지, 검증된 `dev`만 반영 |
| `dev` | 팀 통합 브랜치 | PR 리뷰와 관련 테스트 확인 후 병합 |
| 공동 작업 브랜치 | 짧은 구현 브랜치 | `vm/spt`, `vm/lazy-load`, `vm/mmap`처럼 작업 묶음 단위로 사용 |
| 개인 실험 브랜치 | 디버깅 실험 | 최종 코드로 병합하지 않고 근거만 공유 |

개인별 장기 브랜치는 이번 주차의 기본 전략으로 삼지 않는다. 4명이 함께 이해해야 하는 VM 구조가 핵심이므로, 공동 작업 브랜치와 짧은 PR을 중심으로 운영한다.

## 6. 이슈 보드 운영 방식

11-12주차 이슈는 GitHub Projects 보드를 기준으로 관리한다.

Project URL: https://github.com/orgs/Jungle-12-303/projects/4

CSV 기준 이슈는 총 149개이며, 공통 이슈 6개, 학습 이슈 2개, 테스트 이슈 141개로 구성되어 있다. 테스트 이슈에는 VM 필수 테스트뿐 아니라 Project 2, Threads, filesys/base 회귀 테스트와 `cow-simple (Extra)`가 포함되어 있다.

이슈 운영은 사람별 할당보다 작업 단계별 상태 관리에 맞춘다.

| 이슈 묶음 | 운영 방식 |
|------|------|
| 공통 이슈 | 목표, AI 원칙, WIL, 협업 룰을 4명이 함께 합의 |
| 학습 이슈 | GitBook Project 3, FAQ, Appendix를 공통 세션에서 읽고 질문 정리 |
| VM 구현 테스트 | 구현 순서에 따라 `pt-*`, `page-*`, `mmap-*`, `lazy-*`, `swap-*`로 묶어 처리 |
| Project 2 회귀 테스트 | VM 적용 후 userprog 기능이 깨지지 않는지 단계별 확인 |
| Threads 회귀 테스트 | alarm/priority 기반 회귀 확인 |
| filesys/base 테스트 | mmap과 file-backed page 구현 후 파일 시스템 회귀 확인 |
| Extra | 필수 VM 범위 완료 후 `cow-simple`만 별도 판단 |

## 7. 공통 학습 기준

구현 전후로 팀원 4명은 아래 질문에 답할 수 있어야 한다.

- `struct page`와 `struct frame`은 왜 서로를 참조하는가
- SPT에서 페이지를 찾을 때 왜 page-aligned virtual address를 기준으로 삼는가
- `vm_alloc_page_with_initializer()`는 실제 프레임을 바로 할당하지 않고 무엇을 보관하는가
- `lazy_load_segment()`의 aux에는 어떤 정보가 들어가야 하며 언제 해제되는가
- `vm_do_claim_page()`에서 page table mapping과 `swap_in()` 순서는 왜 중요한가
- stack growth 판단에서 `fault_addr`, `rsp`, `USER_STACK`, stack limit을 어떻게 사용해야 하는가
- mmap된 페이지가 dirty이면 언제 파일에 write-back해야 하는가
- swap slot은 언제 할당하고 언제 해제해야 하는가
- fork 시 부모의 SPT와 fd table을 어떤 기준으로 복제해야 하는가

## 8. PR, 리뷰, 병합 기준

모든 PR에는 최소한 아래 내용이 들어가야 한다.

- 관련 이슈 또는 테스트 이름
- 수정한 파일과 핵심 함수
- 통과한 테스트와 아직 남은 테스트
- 구현 의도와 설계 가정
- 새로 추가한 구조체 필드와 lifecycle
- page, frame, swap slot, file object 중 어떤 자원을 소유하거나 해제하는지
- VM 적용 후 Project 2 회귀 가능성
- 임시 구현이 있다면 임시 범위와 제거 계획

`dev` 병합 기준은 다음과 같다.

- 관련 테스트가 재현 가능하게 통과해야 한다.
- Driver와 공동 검토자가 수정 이유와 동작 원리를 설명할 수 있어야 한다.
- 작성 세션에 참여한 4명 중 Driver가 아니었던 팀원 1명 이상이 PR에서 리뷰 근거를 남겨야 한다.
- `vm.c`, `process.c`, `syscall.c`, `exception.c`, `thread.h`, `include/vm/*.h` 변경은 4명이 함께 핵심 흐름을 확인한다.
- SPT, frame table, swap table처럼 전역 정책이 걸린 변경은 별도 PR로 작게 올린다.
- Project 2 회귀 테스트가 깨질 가능성이 있으면 병합 전에 회귀 범위를 명시한다.

다음 경우에는 병합하지 않는다.

- 테스트는 통과했지만 왜 통과하는지 설명하지 못하는 경우
- 한 명만 이해하는 구조체 필드나 cleanup 경로가 생긴 경우
- page fault 처리에서 invalid access와 recoverable fault 구분이 모호한 경우
- frame, swap slot, file object, aux 메모리 해제 책임이 불명확한 경우
- AI 제안이나 외부 코드를 이해 없이 붙인 경우

## 9. 커밋 메시지와 PR 작성 규칙

커밋 메시지의 기본 형식과 type 기준은 반 공통 커밋 컨벤션을 따른다.

참고: https://github.com/Jungle-12-303/skills/blob/main/commit-convention/SKILL.md

단, 브랜치 전략과 병합 정책은 이 저장소의 협업 문서와 `.github/workflows/branch-policy.yml`을 우선한다.

- 커밋은 한 가지 의도만 담고, 기능명이나 테스트명을 포함해 작성한다.
- 커밋 메시지는 `<type>: <한국어 제목>` 형식을 따른다.
- 커밋 제목에는 스코프를 사용하지 않는다. 영향 범위는 제목과 PR 본문에서 설명한다.
- 허용 type은 `feat`, `fix`, `refactor`, `docs`, `test`, `chore`, `style`, `perf`, `build`, `ci`, `revert`이다.
- 제목은 한국어 한 줄로 작성하고 마침표로 끝내지 않는다.
- 제목은 행위가 아니라 변경 결과를 구체적으로 설명한다.
- `수정`, `작업`, `변경`, `업데이트`, `update`, `fix`처럼 의미가 약한 표현만 단독으로 쓰지 않는다.
- 커밋 예시: `feat: 보조 페이지 테이블을 해시 기반으로 초기화`
- 커밋 예시: `feat: ELF 세그먼트를 지연 로딩 페이지로 등록`
- 커밋 예시: `fix: 스택 확장 주소 검증 범위를 보완`
- 커밋 예시: `docs: 11-12주차 VM 구현 계획을 정리`

## 10. 테스트 운영 원칙

테스트는 "끝났는지 체크하는 용도"가 아니라 "설계 가설이 맞는지 확인하는 도구"로 사용한다.

- 실패 테스트를 공유할 때는 테스트 이름, 재현 명령, 기대 결과, 실제 결과, 최근 수정 파일을 함께 남긴다.
- 한 번에 여러 테스트가 깨지면 구현 순서상 앞선 단계부터 본다.
- `pt-*`가 실패하면 page fault, stack growth, writable 검증을 먼저 본다.
- `page-*`가 실패하면 lazy loading, SPT copy, frame eviction, fork 관계를 함께 본다.
- `mmap-*`가 실패하면 syscall 인자 검증, fd/file 복제, file-backed page, dirty write-back을 함께 본다.
- `lazy-*`가 실패하면 `load_segment()`, `lazy_load_segment()`, aux lifetime을 먼저 본다.
- `swap-*`가 실패하면 frame victim 선정, swap slot bitmap, swap in/out cleanup을 함께 본다.
- Project 2 회귀가 깨지면 VM 변경이 user pointer validation, fork, exec, fd table에 준 영향을 먼저 확인한다.

## 11. AI 활용 원칙

AI 도구는 개념 정리, GitBook 요약, 테스트 실패 가설 정리, 코드 읽기 보조에 활용한다. 단, AI가 제시한 구현을 그대로 병합하지 않는다.

- AI 답변은 반드시 GitBook, 현재 코드, 테스트 결과로 검증한다.
- AI가 작성한 코드라도 팀원이 메모리 소유권과 해제 경로를 설명할 수 있어야 한다.
- 구현 PR에는 "AI가 제안했다"가 아니라 "왜 이 설계가 맞는지"를 적는다.
- AI 사용으로 생성한 문서나 설명은 팀 공통 이해를 돕는 자료로만 사용한다.

## 12. 주간 공유와 제출물

공지 기준 주요 일정은 다음과 같다. 날짜는 2026-05-08(금) 시작 기준으로 정리한다.

| 항목 | 일정 | 제출 또는 준비 내용 |
|------|------|------|
| 첫날 제출 | 2026-05-08(금) 자정까지 | 팀 GitHub Projects 주소, 팀 GitHub repository 주소 |
| Pintos VM 특강 | 2026-05-08(금) 오전 10시 | VM 기초 특강 참여 |
| 퀴즈 | 2026-05-12(화) 오후 2시-3시 | Pintos, C언어 |
| 주간 공유 발표 | 2026-05-14(목), 2026-05-21(목) 오전 10시 | 팀별 발표 자료 1개, 노트북 1대, 개인별 2분 발표 |
| 최종 제출 | 2026-05-21(목) 정오까지 | 주간 공유 발표 자료, WIL |
| 차주 발제 | 2026-05-21(목) 오후 1시 | 11-12주차 동료피드백 |
| 운영진 티타임 | 2026-05-21(목) 오후 3시 | 팀 진행 상황 공유 |

주간 공유 발표에는 아래 세 가지를 포함한다.

- 프로젝트 팀 구성
- 프로젝트 구현 및 트러블슈팅
- 프로젝트 회고

WIL은 이번 주 작성한 블로그 URL을 `WEEK11-12` 태그와 함께 등록한다.

## 13. 완료 기준

11-12주차 작업이 잘 진행되고 있다고 볼 수 있는 기준은 다음과 같다.

- 팀원 4명이 VM 핵심 구조를 같은 용어로 설명할 수 있다.
- SPT, frame, swap, mmap, stack growth 구현의 자원 lifecycle이 문서와 코드에 일관되게 남아 있다.
- VM 필수 테스트를 단계별로 통과하고, Project 2와 Threads 회귀 범위를 확인한다.
- PR에는 테스트 결과뿐 아니라 실패 원인, 설계 근거, 남은 리스크가 함께 남는다.
- 발표 자료와 WIL은 마지막 날에 급히 만들지 않고 PR, 테스트 기록, 트러블슈팅 기록을 재사용한다.
- Extra는 필수 VM 범위가 안정된 뒤에만 판단한다.
