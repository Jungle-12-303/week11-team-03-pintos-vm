# 02_vm_management

## 메모리 관리

가상 메모리 시스템을 지원하려면 가상 페이지와 물리 프레임을 효과적으로 관리해야
합니다. 즉, 어떤 (가상 또는 물리) 메모리 영역이 사용 중인지, 어떤 용도로
쓰이는지, 누가 사용 중인지 등을 추적해야 합니다. 먼저 보조 페이지 테이블을
다루고, 그다음 물리 프레임을 다루겠습니다. 이해를 돕기 위해 여기서는 가상
페이지를 "page", 물리 페이지를 "frame"이라고 부르겠습니다.

### 페이지 구조와 연산

#### struct page

`include/vm/vm.h`에 정의된 `page`는 가상 메모리의 페이지를 나타내는
구조체입니다. 페이지에 대해 알아야 할 모든 필수 데이터를 저장합니다. 현재
템플릿에서 이 구조체는 다음과 같습니다.

```c
struct page {
  const struct page_operations *operations;
  void *va;              /* Address in terms of user space */
  struct frame *frame;   /* Back reference for frame */

  union {
    struct uninit_page uninit;
    struct anon_page anon;
    struct file_page file;
#ifdef EFILESYS
    struct page_cache page_cache;
#endif
  };
};
```

이 구조체에는 페이지 연산(아래 설명 참고), 가상 주소, 물리 프레임이 들어
있습니다. 또한 union 필드가 하나 있습니다. union은 하나의 메모리 영역에 여러
종류의 데이터를 저장할 수 있게 해 주는 특별한 자료형입니다. union에는 여러
멤버가 있지만, 한 시점에는 하나의 멤버만 값을 가질 수 있습니다. 즉, 우리
시스템의 페이지는 `uninit_page`, `anon_page`, `file_page`, `page_cache`
중 하나가 될 수 있습니다. 예를 들어 페이지가 익명 페이지(참고:
[익명 페이지](03_anon.md))라면, 그 page struct는 멤버 중 하나로
`struct anon_page anon` 필드를 가지게 됩니다. `anon_page`에는 익명
페이지에 대해 유지해야 하는 모든 정보가 들어가게 됩니다.

#### 페이지 연산

위에서 설명했듯이, `include/vm/vm.h`에 정의된 페이지는 `VM_UNINIT`,
`VM_ANON`, `VM_FILE` 중 하나가 될 수 있습니다. 페이지에 대해서는 swap in,
swap out, destroy 같은 여러 동작이 필요합니다. 그런데 이런 동작을 수행할 때
필요한 절차와 작업은 페이지 타입마다 다릅니다. 다시 말해 `VM_ANON`
페이지와 `VM_FILE` 페이지에는 서로 다른 `destroy` 함수가 호출되어야 합니다.
한 가지 방법은 각 함수 안에서 switch-case 구문을 사용해 경우를 나누는
것입니다. 여기서는 이를 다루기 위해 객체 지향 프로그래밍의 "클래스 상속"
개념을 도입합니다. 실제로 C 언어에는 "class"도 "inheritance"도 없지만,
[function pointers](https://www.geeksforgeeks.org/function-pointer-in-c/)
를 활용하면 Linux 같은 실제 운영체제 코드와 비슷한 방식으로 이 개념을 구현할
수 있습니다.

함수 포인터는 지금까지 배운 다른 포인터와 마찬가지로 포인터이지만, 함수 또는
메모리 안의 실행 가능한 코드를 가리킨다는 점이 다릅니다. 함수 포인터가
유용한 이유는 별도의 검사 없이 런타임 값에 따라 특정 함수를 간단히 호출해
실행할 수 있기 때문입니다. 우리 경우에는 코드 수준에서 단순히
`destroy (page)`를 호출하면 충분하며, 적절한 함수 포인터를 통해 페이지
타입에 맞는 `destroy` 루틴이 호출됩니다.

페이지 연산을 위한 구조체 `struct page_operations`는 `include/vm/vm.h`에
정의되어 있습니다. 이 구조체는 3개의 함수 포인터가 들어 있는 함수 테이블로
생각하면 됩니다.

```c
struct page_operations {
  bool (*swap_in) (struct page *, void *);
  bool (*swap_out) (struct page *);
  void (*destroy) (struct page *);
  enum vm_type type;
};
```

이제 page_operation 구조체를 어디에서 찾을 수 있는지 보겠습니다.
`include/vm/vm.h`의 `struct page`를 보면 `operations`라는 필드가 있습니다.
이제 `vm/file.c`를 보면 함수 프로토타입 앞쪽에 선언된 page_operations 구조체
`file_ops`를 볼 수 있습니다. 이것이 파일 기반 페이지를 위한 함수 포인터
테이블입니다. `.destroy` 필드에는 `file_backed_destroy`가 들어 있으며, 이
함수는 같은 파일에 정의되어 있고 해당 페이지를 파괴합니다.

함수 포인터 인터페이스를 통해 `file_backed_destroy`가 어떻게 호출되는지
이해해 봅시다. `vm/vm.c`의 `vm_dealloc_page (page)`가 호출되었고, 이
페이지가 파일 기반 페이지(`VM_FILE`)라고 가정합시다. 함수 내부에서는
`destroy (page)`를 호출합니다. `destroy (page)`는 `include/vm/vm.h`에서
다음과 같은 매크로로 정의되어 있습니다.

```c
#define destroy(page) if ((page)->operations->destroy) (page)->operations->destroy (page)
```

즉, `destroy` 함수를 호출하면 실제로는
`(page)->operations->destroy (page)`가 호출되며, 이는 page 구조체에서
가져온 `destroy` 함수입니다. 이 페이지가 `VM_FILE` 페이지이므로 `.destroy`
필드는 `file_backed_destory`를 가리킵니다. 그 결과 파일 기반 페이지를 위한
destroy 루틴이 수행됩니다.

### 보조 페이지 테이블 구현

이 시점의 Pintos에는 메모리의 가상-물리 매핑을 관리하는 페이지 테이블(`pml4`)
이 있습니다. 하지만 이것만으로는 충분하지 않습니다. 앞 절에서 설명했듯이,
페이지 폴트 처리와 자원 관리를 위해 각 페이지의 추가 정보를 담는 보조 페이지
테이블도 필요합니다. 따라서 project 3의 첫 번째 작업으로, 보조 페이지
테이블을 위한 기본 기능들을 먼저 구현할 것을 권장합니다.

**​`vm/vm.c`​**​**에서 보조 페이지 테이블 관리 함수를 구현하세요.**

먼저 여러분의 Pintos에서 보조 페이지 테이블을 어떤 구조로 설계할지 결정해야
합니다. 설계를 마친 뒤, 그 설계에 맞춰 아래 세 함수를 구현하세요.

---

```c
void supplemental_page_table_init (struct supplemental_page_table *spt);
```

> 보조 페이지 테이블을 초기화합니다. 어떤 자료구조를 사용할지는 여러분이
> 선택할 수 있습니다. 이 함수는 새 프로세스가 시작할 때(`userprog/process.c`
> 의 `initd`)와 프로세스를 fork할 때(`userprog/process.c`의 `__do_fork`)
> 호출됩니다.

---

```c
struct page *spt_find_page (struct supplemental_page_table *spt, void *va);
```

> 주어진 보조 페이지 테이블에서 `va`에 대응하는 `struct page`를 찾습니다.
> 실패하면 NULL을 반환합니다.

---

```c
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
```

> 주어진 보조 페이지 테이블에 `struct page`를 삽입합니다. 이 함수는 주어진
> 보조 페이지 테이블에 해당 가상 주소가 이미 존재하지 않는지 확인해야 합니다.

### 프레임 관리

이제부터 페이지는 단지 생성 시점의 메타데이터만 들고 있는 것이 아닙니다.
따라서 물리 메모리를 관리하기 위한 다른 방식이 필요합니다. `include/vm/vm.h`
에는 물리 메모리를 나타내는 `struct frame`이 있습니다. 현재 구조체는 다음과
같습니다.

```c
/* The representation of "frame" */
struct frame {
  void *kva;
  struct page *page;
};
```

이 구조체에는 `kva`와 `page`, 이렇게 두 필드만 있습니다. `kva`는 커널 가상
주소이고, `page`는 page 구조체입니다. 프레임 관리 인터페이스를 구현하면서
필요하다면 멤버를 더 추가해도 됩니다.

**​`vm/vm.c`​**​**에서 **​**​`vm_get_frame`​**​ **, **​**​`vm_claim_page`​**​ **, **​**​`vm_do_claim_page`​**​**를**​**구현하세요.**

---

```c
static struct frame *vm_get_frame (void);
```

> `palloc_get_page`를 호출해 user pool에서 새 물리 페이지를 가져옵니다.
> user pool에서 페이지를 성공적으로 얻었다면 frame도 할당하고, 그 멤버들을
> 초기화한 뒤 반환합니다.
> `vm_get_frame`을 구현한 뒤에는 모든 사용자 공간 페이지(`PALLOC_USER`)를
> 이 함수를 통해 할당해야 합니다. 지금 단계에서는 페이지 할당 실패 시 swap
> out을 처리할 필요는 없습니다. 그런 경우는 일단 `PANIC ("todo")`로
> 표시해 두세요.

---

```c
bool vm_do_claim_page (struct page *page);
```

> 페이지를 claim한다는 것은 해당 페이지에 물리 프레임을 할당하는 뜻입니다.
> 먼저 `vm_get_frame`을 호출해 프레임을 얻습니다(이 부분은 템플릿에서 이미
> 제공됩니다). 그런 다음 MMU를 설정해야 합니다. 즉, 페이지 테이블에 가상
> 주소에서 물리 주소로의 매핑을 추가해야 합니다. 반환값은 이 작업이
> 성공했는지 여부를 나타내야 합니다.

---

```c
bool vm_claim_page (void *va);
```

> `va`를 할당하기 위해 해당 페이지를 claim합니다. 먼저 페이지를 찾아야 하고,
> 그다음 그 페이지를 인자로 `vm_do_claim_page`를 호출합니다.
