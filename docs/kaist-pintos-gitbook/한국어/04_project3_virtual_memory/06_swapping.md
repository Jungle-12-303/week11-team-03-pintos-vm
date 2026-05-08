# 06_swapping

## 스왑 인/아웃

메모리 스와핑은 물리 메모리 사용량을 최대화하기 위한 메모리 회수 기법입니다.
주 메모리의 프레임이 모두 할당되면, 시스템은 사용자 프로그램의 추가 메모리
할당 요청을 더 이상 처리할 수 없습니다. 한 가지 해결책은 현재 사용되지 않는
메모리 프레임을 디스크로 swap out하는 것입니다. 이렇게 하면 일부 메모리
자원이 해제되어 다른 애플리케이션이 사용할 수 있게 됩니다.

스와핑은 운영체제가 수행합니다. 시스템이 메모리를 모두 사용한 상태에서 메모리
할당 요청을 받으면, OS는 어떤 페이지를 스왑 디스크로 축출할지 선택합니다.
그다음 메모리 프레임의 정확한 상태를 디스크에 복사합니다. 이후 프로세스가
swap out된 페이지에 접근하려고 하면, OS는 그 정확한 내용을 다시 메모리로
가져와 페이지를 복구합니다.

축출 대상으로 선택된 페이지는 익명 페이지일 수도 있고 파일 기반 페이지일 수도
있습니다. 이 절에서는 각 경우를 모두 다룹니다.

스와핑 관련 연산은 명시적으로 호출되지 않고 함수 포인터 형태로 호출됩니다.
이 연산들은 `struct page_operations file_ops`의 멤버이며, 각 페이지의
initializer가 등록하는 연산으로 사용됩니다.

### 익명 페이지

**​`vm/anon.c`​**​**에서 **​**​`vm_anon_init`​**​**와 **​**​`anon_initializer`​**​**를 수정하세요.** 
익명 페이지는 이를 위한 backing storage를 따로 갖지 않습니다. 익명 페이지의
스와핑을 지원하기 위해, 임시 backing storage인 swap disk를 제공합니다.
익명 페이지 스왑을 구현할 때 이 swap disk를 사용하게 됩니다.

---

```c
void vm_anon_init (void);
```

> 이 함수에서는 swap disk를 설정해야 합니다. 또한 swap disk 안의 사용 중인
> 영역과 비어 있는 영역을 관리할 자료구조도 필요합니다. swap 영역 역시
> PGSIZE(4096바이트) 단위로 관리됩니다.

---

```c
bool anon_initializer (struct page *page, enum vm_type type, void *kva);
```

> 익명 페이지를 위한 initializer입니다. 스와핑을 지원하기 위해 `anon_page`에
> 어떤 정보를 추가해야 합니다.

이제 `vm/anon.c`에서 `anon_swap_in`과 `anon_swap_out`을 구현해 익명 페이지
스와핑을 지원하세요. 어떤 페이지가 swap in되려면 먼저 swap out된 상태여야
하므로, `anon_swap_in`보다 `anon_swap_out`을 먼저 구현하는 편이 좋을 수
있습니다. 데이터 내용을 스왑 디스크로 옮기고, 다시 안전하게 메모리로 가져와야
합니다.

---

```c
static bool anon_swap_in (struct page *page, void *kva);
```

> 스왑 디스크에서 데이터를 읽어 메모리로 복원함으로써 익명 페이지를 swap in
> 합니다. 데이터가 있는 위치는 페이지가 swap out될 때 page struct에 저장해
> 두었어야 합니다. swap table도 갱신해야 한다는 점을 잊지 마세요
> ([스왑 테이블 관리](01_introduction.md) 참고).

---

```c
static bool anon_swap_out (struct page *page);
```

> 메모리의 내용을 디스크에 복사해 익명 페이지를 swap out합니다. 먼저 swap
> table을 사용해 디스크에서 비어 있는 스왑 슬롯을 찾은 뒤, 해당 슬롯에
> 페이지 데이터를 복사하세요. 데이터 위치는 page struct에 저장해야 합니다.
> 디스크에 빈 슬롯이 더 이상 없다면 커널을 panic 시켜도 됩니다.

### 파일 매핑 페이지

파일 기반 페이지의 내용은 파일에서 오므로, mmap된 파일 자체를 backing store로
사용해야 합니다. 즉, 파일 기반 페이지를 축출할 때는 그 페이지를 매핑한
원본 파일에 내용을 다시 써야 합니다. `vm/file.c`에서
`file_backed_swap_in`, `file_backed_swap_out`을 구현하세요. 설계에 따라
`file_backed_init`와 `file_initializer`를 수정할 수도 있습니다.

---

```c
static bool file_backed_swap_in (struct page *page, void *kva);
```

> 파일의 내용을 읽어 와 `kva`에 있는 페이지를 swap in합니다. 파일 시스템과
> 동기화해야 합니다.

---

```c
static bool file_backed_swap_out (struct page *page);
```

> 페이지 내용을 파일에 다시 기록해 swap out합니다. 먼저 페이지가 dirty한지
> 확인하는 것이 좋습니다. dirty하지 않다면 파일 내용을 수정할 필요가
> 없습니다. 페이지를 swap out한 뒤에는 해당 페이지의 dirty 비트를 끄는 것도
> 잊지 마세요.
