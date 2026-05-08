# 01_threads

[]()</a>

# 스레드

[]()</a>

## `struct thread`

Pintos에서 스레드를 위한 주요 자료구조는 `threads/thread.h`에 선언된 `struct thread`입니다.

---

```c
struct thread;
```

스레드 또는 사용자 프로세스를 나타냅니다. 프로젝트를 진행하면서 `struct thread`에 여러분이 직접 멤버를 추가해야 합니다. 기존 멤버의 정의를 변경하거나 삭제해도 됩니다. 각 `struct thread`는 자신에게 할당된 메모리 페이지의 시작 부분을 차지합니다. 페이지의 나머지 부분은 스레드 스택으로 사용되며, 스택은 페이지 끝에서 아래 방향으로 자랍니다. 형태는 다음과 같습니다:

```
                  4 kB +---------------------------------+
                       |         kernel stack            |
                       |               |                 |
                       |               |                 |
                       |               V                 |
                       |        grows downward           |
                       |                                 |
                       |                                 |
                       |                                 |
                       |                                 |
                       |                                 |
                       |                                 |
                       |                                 |
                       |                                 |
sizeof (struct thread) +---------------------------------+
                       |             magic               |
                       |          intr_frame             |
                       |               :                 |
                       |               :                 |
                       |             status              |
                       |              tid                |
                  0 kB +---------------------------------+
```

이 구조에는 두 가지 결과가 따릅니다. 첫째, `struct thread`가 너무 커지면 안 됩니다. 너무 커지면 커널 스택을 위한 공간이 부족해집니다. 기본 `struct thread`는 크기가 몇 바이트에 불과합니다. 아마도 1 kB보다 충분히 작게 유지하는 편이 좋습니다. 둘째, 커널 스택도 너무 크게 자라면 안 됩니다. 스택이 오버플로되면 스레드 상태를 손상시킵니다. 따라서 커널 함수는 큰 구조체나 배열을 비정적 지역 변수로 할당해서는 안 됩니다. 대신 `malloc()`이나 `palloc_get_page()`를 사용한 동적 할당을 사용하십시오([메모리 할당](03_memory_allocation.md) 참고).

---

```c
tid_t tid;
```

> 스레드 식별자, 즉 tid입니다. 모든 스레드는 커널의 전체 수명 동안 유일한 tid를 가져야 합니다. 기본적으로 `tid_t`는 `int`에 대한 `typedef`이며, 각 새 스레드는 초기 프로세스의 1부터 시작해 그보다 1 큰 숫자의 tid를 차례대로 받습니다. 원한다면 타입과 번호 부여 방식을 변경해도 됩니다.

---

```c
enum thread_status status;
```

> 래퍼 함수 비슷한거 하나 더 있음
> 
> 스레드의 상태입니다. 다음 중 하나를 가집니다:
>
> - `THREAD_RUNNING`
>
>   스레드가 실행 중인 상태입니다. 한 시점에는 정확히 하나의 스레드만 실행됩니다. `thread_current()`는 현재 실행 중인 스레드를 반환합니다.
> - `THREAD_READY`
>
>   스레드는 실행할 준비가 되어 있지만 지금 당장은 실행 중이 아닙니다. 스케줄러가 다음에 호출될 때 선택될 수 있습니다. 준비 상태의 스레드는 `ready_list`라는 이중 연결 리스트에 보관됩니다.
> - `THREAD_BLOCKED`
>
>   스레드는 어떤 것을 기다리고 있는 상태입니다. 예를 들어 락이 사용 가능해지거나 인터럽트가 발생하기를 기다릴 수 있습니다. `thread_unblock()` 호출로 `THREAD_READY` 상태로 전이하기 전까지는 다시 스케줄되지 않습니다. 이는 보통 스레드를 자동으로 차단하고 깨워 주는 Pintos 동기화 프리미티브를 통해 간접적으로 처리하는 것이 가장 편합니다([동기화](02_synchronization.md) 참고). 차단된 스레드가 무엇을 기다리는지 *사전에* 알 수 있는 방법은 없지만, 백트레이스(backtrace)가 도움이 될 수 있습니다([백트레이스](06_debugging_tools.md#backtraces) 참고).
> - `THREAD_DYING`
>
>   스케줄러가 다음 스레드로 전환한 뒤 이 스레드를 파괴합니다.

---

```c
char name[16];
```

> 스레드의 이름을 담는 문자열입니다. 또는 적어도 이름의 앞부분 몇 글자를 담습니다.

---

```c
struct intr_frame tf;
```

> 레지스터와 스택 포인터를 포함해 컨텍스트 스위칭에 필요한 정보를 저장합니다.

---

```c
int priority;
```

> 스레드 우선순위입니다. 범위는 `PRI_MIN` (0)부터 `PRI_MAX` (63)까지입니다. 숫자가 낮을수록 우선순위가 낮으므로, 우선순위 0이 가장 낮고 우선순위 63이 가장 높습니다. 기본 Pintos는 스레드 우선순위를 무시하지만, project 1에서 우선순위 스케줄링을 구현하게 됩니다([우선순위 스케줄링](../02_project1_threads/03_priority_scheduling.md) 참고).

---

```c
struct list_elem elem;
```

> `ready_list`(실행 준비가 된 스레드 목록)나 `sema_down()`에서 세마포어를 기다리는 스레드 목록처럼, 이중 연결 리스트에 스레드를 넣기 위해 사용하는 "list element"입니다. 세마포어를 기다리는 스레드는 준비 상태가 아니고, 그 반대도 마찬가지이므로 하나의 멤버로 두 역할을 겸할 수 있습니다.

---

```c
uint64_t *pml4;
```

> project 2 이후에만 존재합니다. [페이지 테이블](04_virtual_address.md)을 참고하십시오.

---

```c
unsigned magic
```

> 항상 `THREAD_MAGIC`으로 설정됩니다. 이는 `threads/thread.c`에 정의된 임의의 숫자일 뿐이며, 스택 오버플로를 감지하는 데 사용됩니다. `thread_current()`는 실행 중인 스레드의 `struct thread`에서 `magic` 멤버가 `THREAD_MAGIC`으로 설정되어 있는지 확인합니다. 스택 오버플로가 발생하면 이 값이 바뀌는 경향이 있어서 assertion이 실패합니다. 최대한 효과를 얻으려면 `struct thread`에 멤버를 추가할 때 `magic`을 끝에 남겨 두십시오.

[]()</a>

## 스레드 함수

`threads/thread.c`는 스레드 지원을 위한 여러 공개 함수를 구현합니다. 그중 가장 유용한 것들을 살펴보겠습니다:

---

```c
void thread_init (void);
```

> `main()`이 스레드 시스템을 초기화하기 위해 호출합니다. 주된 목적은 Pintos의 초기 스레드에 대한 `struct thread`를 만드는 것입니다. Pintos 로더가 초기 스레드의 스택을 다른 Pintos 스레드와 같은 위치, 즉 페이지 맨 위에 배치하므로 이것이 가능합니다.
>
> `thread_init()`가 실행되기 전에는 실행 중인 스레드의 `magic` 값이 올바르지 않기 때문에 `thread_current()`가 실패합니다. 락을 획득하는 `lock_acquire()`를 포함해 많은 함수가 직접 또는 간접적으로 `thread_current()`를 호출하므로, `thread_init()`은 Pintos 초기화 초기에 호출됩니다.

---

```c
void thread_start (void);
```

> `main()`이 스케줄러를 시작하기 위해 호출합니다. 다른 어떤 스레드도 실행 준비 상태가 아닐 때 스케줄되는 idle thread를 생성합니다. 이어서 인터럽트를 활성화하는데, 타이머 인터럽트에서 복귀할 때 `intr_yield_on_return()`을 사용해 스케줄러가 동작하므로 이 부수 효과로 스케줄러도 활성화됩니다.

---

```c
void thread_tick (void);
```

> 매 타이머 틱마다 타이머 인터럽트가 호출합니다. 스레드 통계를 추적하고 타임 슬라이스(time slice)가 만료되면 스케줄러를 트리거합니다.

---

```c
void thread_print_stats (void);
```

> Pintos 종료 시 스레드 통계를 출력하기 위해 호출됩니다.

---

```c
tid_t thread_create (const char *name, int priority, thread func *func, void *aux);
```

> name이라는 이름과 주어진 priority를 가진 새 스레드를 만들고 시작한 뒤, 새 스레드의 tid를 반환합니다. 스레드는 func를 실행하며 aux를 함수의 단일 인자로 전달받습니다.
>
> `thread_create()`는 스레드의 `struct thread`와 stack을 위해 한 페이지를 할당하고 멤버를 초기화한 뒤, 그 스레드를 위한 가짜 stack frame 집합을 설정합니다. 스레드는 차단 상태로 초기화되며, 반환 직전에 깨워져 스케줄될 수 있게 됩니다.

---

```c
void thread_func (void *aux);
```

> `thread_create()`에 전달되는 함수의 타입입니다. aux 인자는 그 함수의 인자로 전달됩니다.

---

```c
void thread_block (void);
```

> 현재 실행 중인 스레드를 running 상태에서 차단 상태로 전이시킵니다. 이 스레드는 `thread_unblock()`이 호출될 때까지 다시 실행되지 않으므로, 그 일이 일어나도록 미리 적절한 장치를 마련해 두어야 합니다. `thread_block()`은 매우 저수준이므로, 대신 동기화 프리미티브 중 하나를 사용하는 편이 좋습니다([동기화](02_synchronization.md) 참고).

---

```c
void thread_unblock (struct thread *thread);
```

> 차단 상태여야 하는 thread를 실행 준비 상태로 전이시켜 다시 실행될 수 있게 합니다. 예를 들어 스레드가 기다리던 lock이 사용 가능해졌을 때처럼, 해당 이벤트가 발생하면 호출됩니다.

---

```c
struct thread *thread_current (void);
```

> 현재 실행 중인 스레드를 반환합니다.

---

```c
tid_t thread_tid (void);
```

> 현재 실행 중인 스레드의 thread id를 반환합니다.
> `thread_current ()->tid`와 같습니다.

---

```c
const char *thread_name (void);
```

> 현재 실행 중인 스레드의 이름을 반환합니다. `thread_current ()->name`과 같습니다.

---

```c
void thread_exit (void) NO_RETURN;
```

> 현재 스레드를 종료시킵니다. 반환하지 않습니다.

---

```c
void thread_yield (void);
```

> CPU를 스케줄러에 양보하여 새로 실행할 스레드를 고르게 합니다. 새 스레드가 현재 스레드일 수도 있으므로, 이 함수가 현재 스레드를 특정 시간 동안 실행되지 않게 보장한다고 기대해서는 안 됩니다.

---

```c
int thread_get_priority (void);
void thread_set_priority (int new_priority);
```

> 스레드 우선순위를 설정하고 가져오기 위한 stub 구현입니다. [우선순위 스케줄링](../02_project1_threads/03_priority_scheduling.md)을 참고하십시오.

---

```c
int thread_get_nice (void);
void thread_set_nice (int new_nice);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);
```

> 고급 스케줄러를 위한 stub 구현입니다.
