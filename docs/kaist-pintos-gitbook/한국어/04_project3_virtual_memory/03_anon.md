# 03_anon

## 익명 페이지

이 프로젝트의 이 부분에서는 디스크 기반 이미지가 아닌 *익명 페이지*를
구현합니다.

익명 매핑은 backing file이나 장치를 가지지 않습니다. 파일 기반 페이지와 달리
이름 붙은 파일 소스가 없기 때문에 anonymous라고 부릅니다. 익명 페이지는 실행
이미지의 메모리, 예를 들어 스택과 힙에서 사용됩니다.

익명 페이지를 설명하는 구조체로 `include/vm/anon.h`의 `anon_page`가
있습니다. 현재는 비어 있지만, 구현 과정에서 익명 페이지의 필요한 정보나 상태를
저장하도록 멤버를 추가할 수 있습니다. 또한 `include/vm/page.h`의
`struct page`도 참고하세요. 이 구조체에는 페이지의 일반적인 정보가 들어
있습니다. 익명 페이지의 경우, 해당 page 구조체에는 `struct anon_page anon`이
포함됩니다.

### 지연 로딩을 통한 페이지 초기화

지연 로딩(lazy loading)은 실제로 필요해질 때까지 메모리 로딩을 미루는
설계입니다. 페이지는 할당되며, 즉 그에 대응하는 page struct는 존재하지만,
전용 물리 프레임은 아직 없고 실제 내용도 아직 로드되지 않은 상태입니다.
내용은 정말로 필요할 때만 로드되며, 그 시점은 페이지 폴트로 나타납니다.

페이지 타입이 세 가지이므로 초기화 루틴도 타입마다 다릅니다. 이 내용은 아래
절들에서 다시 설명하지만, 여기서는 페이지 초기화 흐름을 상위 수준에서 먼저
보겠습니다. 먼저 커널이 새 페이지 요청을 받으면
`vm_alloc_page_with_initializer`가 호출됩니다. initializer는 page 구조체를
할당하고 페이지 타입에 맞는 적절한 initializer를 설정한 뒤, 제어를 사용자
프로그램으로 되돌려 줍니다. 사용자 프로그램이 실행되다가, 자신이 가지고 있다고
생각하는 페이지에 접근했지만 아직 그 페이지에 내용이 없어서 어느 시점에
페이지 폴트가 발생합니다. 폴트 처리 과정에서는 `uninit_initialize`가 호출되고,
이 함수는 여러분이 앞에서 설정한 initializer를 다시 호출합니다. 익명 페이지의
경우 initializer는 `anon_initializer`, 파일 기반 페이지의 경우
`file_backed_initializer`가 됩니다.

페이지는 initialize->(page_fault->lazy-load->swap-in>swap-out->...)->destroy
와 같은 생명 주기를 가질 수 있습니다. 생명 주기의 각 전이마다 필요한 절차는
페이지 타입(또는 `VM_TYPE`)에 따라 달라지며, 앞 단락은 그중 초기화에 대한
예시였습니다. 이번 프로젝트에서는 각 페이지 타입마다 이러한 전이 과정을
구현하게 됩니다.

### 실행 파일을 위한 지연 로딩

지연 로딩에서는 프로세스가 실행을 시작할 때 당장 필요한 메모리 부분만 주
메모리에 로드됩니다. 이렇게 하면 바이너리 이미지를 한 번에 전부 메모리에
올리는 eager loading에 비해 오버헤드를 줄일 수 있습니다.

지연 로딩을 지원하기 위해 `include/vm/vm.h`에 `VM_UNINIT`이라는 페이지
타입을 도입합니다. 모든 페이지는 처음에 `VM_UNINIT` 페이지로 생성됩니다.
또한 초기화되지 않은 페이지를 위한 page 구조체인 `struct uninit_page`를
`include/vm/uninit.h`에 제공합니다. 초기화되지 않은 페이지를 생성, 초기화,
파괴하는 함수는 `include/vm/uninit.c`에서 찾을 수 있습니다. 이 함수들은
나중에 여러분이 완성해야 합니다.

페이지 폴트가 발생하면 페이지 폴트 핸들러(`userprog/exception.c`의
`page_fault`)는 `vm/vm.c`의 `vm_try_handle_fault`로 제어를 넘기고, 이
함수는 먼저 해당 폴트가 처리 가능한 페이지 폴트인지 검사합니다. 여기서
*valid*란 커널이 복구할 수 있는 페이지 폴트를 뜻합니다. 그런 경우라면,
페이지에 필요한 내용을 로드한 뒤 사용자 프로그램으로 제어를 돌려줍니다.

이런 페이지 폴트에는 세 가지 경우가 있습니다. 지연 로딩된 페이지, swap out된
페이지, 그리고 쓰기 보호된 페이지입니다([Copy-on-Write (추가)](07_cow.md)
참고). 지금은 첫 번째 경우인 지연 로딩 페이지만 생각하면 됩니다. 지연
로딩을 위한 페이지 폴트라면, 커널은 `vm_alloc_page_with_initializer`에서
앞서 설정해 둔 initializer 중 하나를 호출해 세그먼트를 lazy load합니다.
`userprog/process.c`의 `lazy_load_segment`를 구현해야 합니다.

**​`vm_alloc_page_with_initializer()`​**​**를 구현하세요.** 
전달된 vm_type에 맞는 적절한 initializer를 가져오고, 이를 사용해
`uninit_new`를 호출해야 합니다.

---

```c
bool vm_alloc_page_with_initializer (enum vm_type type, void *va,
        bool writable, vm_initializer *init, void *aux);
```

> 주어진 타입의 초기화되지 않은 페이지를 생성합니다. uninit page의
> swap_in 핸들러는 타입에 따라 자동으로 페이지를 초기화하고, 주어진 `AUX`와
> 함께 `INIT`를 호출합니다. page struct를 만든 뒤에는 그 페이지를
> 프로세스의 보조 페이지 테이블에 삽입해야 합니다. `vm.h`에 정의된
> `VM_TYPE` 매크로가 유용할 수 있습니다.

페이지 폴트 핸들러는 호출 체인을 따라가며, swap_in을 호출할 때 결국
`uninit_intialize`에 도달합니다. 이에 대한 완전한 구현은 제공되어 있습니다.
다만, 여러분의 설계에 따라 `uninit_initialize`를 수정해야 할 수도 있습니다.

---

```c
static bool uninit_initialize (struct page *page, void *kva);
```

> 첫 번째 폴트에서 페이지를 초기화합니다. 템플릿 코드는 먼저
> `vm_initializer`와 `aux`를 가져온 뒤, 함수 포인터를 통해 대응하는
> page_initializer를 호출합니다. 여러분의 설계에 따라 이 함수를 수정해야 할
> 수도 있습니다.

필요에 따라 `vm/anon.c`의 `vm_anon_init`와 `anon_initializer`를 수정해도
됩니다.

---

```c
void vm_anon_init (void);
```

> 익명 페이지 서브시스템을 초기화합니다. 이 함수에서는 익명 페이지와 관련된
> 어떤 설정이든 할 수 있습니다.

---

```c
bool anon_initializer (struct page *page,enum vm_type type, void *kva);
```

> 이 함수는 먼저 익명 페이지를 위한 핸들러를 `page->operations`에 설정합니다.
> 현재 비어 있는 `anon_page`에 어떤 정보를 갱신해야 할 수도 있습니다. 이
> 함수는 익명 페이지(즉 `VM_ANON`)의 initializer로 사용됩니다.

**​`userprog/process.c`​**​**에서 **​**​`load_segment`​**​**와 **​**​`lazy_load_segment`​**​**를**​**구현하세요.**  실행 파일로부터 세그먼트를 로드하는 부분입니다. 이 페이지들은
모두 lazy하게 로드되어야 합니다. 즉, 커널이 이 페이지들에 대한 페이지 폴트를
가로챌 때만 실제 로드가 이루어져야 합니다.

프로그램 로더의 핵심인 `userprog/process.c`의 `load_segment` 루프 본문을
수정해야 합니다. 루프를 한 번 돌 때마다 `vm_alloc_page_with_initializer`를
호출해 보류 중인 페이지 객체를 만듭니다. 그리고 페이지 폴트가 발생했을 때
비로소 세그먼트가 실제 파일에서 로드됩니다.

---

```c
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
        uint32_t read_bytes, uint32_t zero_bytes, bool writable);
```

> 현재 코드는 메인 루프 안에서 파일에서 읽을 바이트 수와 0으로 채울 바이트
> 수를 계산합니다. 그런 다음 `vm_alloc_page_with_initializer`를 호출해
> 보류 객체를 생성합니다. 여러분은 `vm_alloc_page_with_initializer`에 전달할
> `aux` 인자로 보조 값을 설정해야 합니다. 바이너리 로딩에 필요한 정보를 담는
> 구조체를 만드는 것이 좋을 수 있습니다.

---

```c
static bool lazy_load_segment (struct page *page, void *aux);
```

> `load_segment`에서 `vm_alloc_page_with_initializer`의 네 번째 인자로
> `lazy_load_segment`가 전달된 것을 눈치챘을 것입니다. 이 함수는 실행 파일
> 페이지를 위한 initializer이며, 페이지 폴트가 발생할 때 호출됩니다.
> 인자로 page struct와 `aux`를 받습니다. `aux`에는 `load_segment`에서
> 설정한 정보가 들어 있습니다. 이 정보를 사용해 세그먼트를 읽어 올 파일을
> 찾아야 하며, 결국 그 세그먼트를 메모리에 읽어 와야 합니다.

**​`userprog/process.c`​**​**의 **​**​`setup_stack`​**​**도 새 메모리 관리 시스템에 맞게**​**조정해야 합니다.**  첫 번째 스택 페이지는 lazy하게 할당할 필요가 없습니다.
이 페이지는 load 시점에 커맨드라인 인자와 함께 곧바로 할당하고 초기화해도
되며, 페이지 폴트를 기다릴 필요가 없습니다. 스택을 식별할 방법도 필요할 수
있습니다. `vm/vm.h`의 `vm_type`에 있는 보조 마커(예: `VM_MARKER_0`)를
사용해 페이지를 표시할 수 있습니다.

마지막으로 `vm_try_handle_fault`를 수정해, 보조 페이지 테이블에서
`spt_find_page`를 통해 폴트가 발생한 주소에 대응하는 page struct를 찾도록
하세요.

이 요구 사항들을 모두 구현하면, project 2의 테스트 중 `fork`를 제외한 모든
테스트를 통과해야 합니다.

### 보조 페이지 테이블 다시 보기

이제 복사와 정리 작업을 지원하기 위해 보조 페이지 테이블 인터페이스를 다시
살펴보겠습니다. 이 작업들은 프로세스를 생성할 때(더 정확히는 자식 프로세스를
만들 때) 또는 프로세스를 파괴할 때 필요합니다. 자세한 내용은 아래에
설명합니다. 지금 이 시점에서 보조 페이지 테이블을 다시 다루는 이유는, 위에서
구현한 초기화 함수들을 여기에서도 활용하고 싶을 수 있기 때문입니다.

**​`vm/vm.c`​**​**에서 **​**​`supplemental_page_table_copy`​**​**와**​**​`supplemental_page_table_kill`​**​**을 구현하세요.**

---

```c
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
    struct supplemental_page_table *src);
```

> 보조 페이지 테이블을 src에서 dst로 복사합니다. 자식이 부모의 실행
> 컨텍스트를 상속받아야 할 때(즉 `fork()`) 사용됩니다. src의 보조 페이지
> 테이블에 있는 각 페이지를 순회하면서, 그 엔트리의 정확한 복사본을 dst의
> 보조 페이지 테이블에 만들어야 합니다. uninit page를 할당하고 이를 즉시
> claim해야 합니다.

---

```c
void supplemental_page_table_kill (struct supplemental_page_table *spt);
```

> 보조 페이지 테이블이 보유하고 있던 모든 자원을 해제합니다. 이 함수는
> 프로세스가 종료할 때(`userprog/process.c`의 `process_exit()`) 호출됩니다.
> 페이지 엔트리들을 순회하면서 테이블 안의 각 페이지에 대해 `destroy(page)`를
> 호출해야 합니다. 이 함수에서는 실제 페이지 테이블(`pml4`)이나 물리 메모리
> (`palloc`으로 할당된 메모리)를 걱정할 필요가 없습니다. 호출자가 보조 페이지
> 테이블 정리가 끝난 뒤 그것들을 정리합니다.

### 페이지 정리

**​`vm/uninit.c`​**​**의 **​**​`uninit_destroy`​**​**와 **​**​`vm/anon.c`​**​**의 **​**​`anon_destroy`​**​**를**​**구현하세요.**  이것은 초기화되지 않은 페이지에 대한 `destroy` 연산 핸들러입니다.
초기화되지 않은 페이지는 다른 페이지 객체로 변환되지만, 프로세스가 종료할 때
여전히 uninit page가 남아 있을 수 있습니다.

---

```c
static void uninit_destroy (struct page *page);
```

> page struct가 보유한 자원을 해제합니다. 페이지의 vm type을 확인한 뒤 그에
> 맞게 처리하고 싶을 수 있습니다.

지금 단계에서는 익명 페이지만 처리하면 됩니다. 파일 기반 페이지 정리를 위해
나중에 이 함수를 다시 살펴보게 됩니다.

---

```c
static void anon_destroy (struct page *page);
```

> 익명 페이지가 보유한 자원을 해제합니다. page struct 자체를 명시적으로
> 해제할 필요는 없습니다. 호출자가 해제해야 합니다.

이제 project 2의 모든 테스트를 통과해야 합니다.
