# 02_synchronization

[]()</a>

# 동기화

스레드 간 자원 공유를 신중하고 통제된 방식으로 다루지 않으면 결과는 대개 큰 혼란이 됩니다. 운영체제 커널에서는 잘못된 공유가 전체 시스템을 다운시킬 수 있으므로 특히 그렇습니다. Pintos는 이를 돕기 위한 여러 동기화 프리미티브를 제공합니다.

[]()</a>

## 인터럽트 비활성화

동기화를 수행하는 가장 거친 방법은 인터럽트를 비활성화하는 것입니다. 즉, CPU가 인터럽트에 응답하지 않도록 일시적으로 막는 것입니다. 인터럽트가 꺼져 있으면 스레드 선점은 타이머 인터럽트에 의해 일어나므로 다른 스레드가 현재 실행 중인 스레드를 선점할 수 없습니다. 반대로 보통처럼 인터럽트가 켜져 있으면, 실행 중인 스레드는 두 개의 C 문장 사이뿐 아니라 하나의 문장이 실행되는 도중에도 언제든 다른 스레드에 의해 선점될 수 있습니다.

덧붙여 말하면, 이는 Pintos가 "선점형 커널(preemptible kernel)"이라는 뜻입니다. 즉, 커널 스레드는 언제든 선점될 수 있습니다. 전통적인 Unix 시스템은 "비선점형(nonpreemptible)"입니다. 즉, 커널 스레드는 스케줄러를 명시적으로 호출하는 지점에서만 선점될 수 있습니다. (사용자 프로그램은 두 모델 모두에서 언제든 선점될 수 있습니다.) 예상할 수 있듯이, 선점형 커널은 더 명시적인 동기화를 요구합니다.

인터럽트 상태를 직접 설정해야 할 일은 거의 없습니다. 대부분의 경우 다음 절에서 설명하는 다른 동기화 프리미티브를 사용하면 됩니다. 인터럽트를 비활성화하는 주된 이유는 커널 스레드를 외부 인터럽트 핸들러와 동기화하기 위해서입니다. 외부 인터럽트 핸들러는 잠들 수 없기 때문에 다른 형태의 동기화를 대부분 사용할 수 없습니다.

일부 외부 인터럽트는 인터럽트를 비활성화해도 미룰 수 없습니다. 이런 인터럽트를 **non-maskable interrupt**(NMI)라고 하며, 컴퓨터에 불이 난 경우처럼 비상 상황에서만 사용하도록 되어 있습니다. Pintos는 non-maskable interrupt를 처리하지 않습니다.

인터럽트를 비활성화하고 다시 활성화하는 타입과 함수는 `include/threads/interrupt.h`에 있습니다.

---

```c
enum intr_level;
```

> 인터럽트가 각각 비활성화 또는 활성화되어 있음을 나타내는 `INTR_OFF` 또는 `INTR_ON` 중 하나입니다.

---

```c
enum intr_level intr_get_level (void)
```

> 현재 인터럽트 상태를 반환합니다.

---

```c
enum intr_level intr_set_level (enum intr_level level);
```

> level에 따라 인터럽트를 켜거나 끕니다. 이전 인터럽트 상태를 반환합니다.

---

```c
enum intr_level intr_enable (void);
```

> 인터럽트를 켭니다. 이전 인터럽트 상태를 반환합니다.

---

```c
enum intr_level intr_disable (void);
```

> 인터럽트를 끕니다. 이전 인터럽트 상태를 반환합니다.

[]()</a>

## 세마포어

세마포어는 음이 아닌 정수와, 그 값을 원자적으로 조작하는 두 연산자로 구성됩니다:

- "Down" 또는 "P": 값이 양수가 될 때까지 기다린 뒤 1 감소시킵니다.
- "UP" 또는 "V": 값을 증가시킵니다(대기 중인 스레드가 있으면 그중 하나를 깨웁니다).

0으로 초기화한 세마포어는 정확히 한 번 발생할 이벤트를 기다리는 데 사용할 수 있습니다. 예를 들어 스레드 A가 다른 스레드 B를 시작시키고, 어떤 작업이 끝났다는 신호를 B가 보내 주기를 기다리고 싶다고 합시다. A는 0으로 초기화한 세마포어를 하나 만들고, 스레드 B를 시작할 때 그것을 넘겨준 뒤 세마포어에 대해 "down"을 수행할 수 있습니다. B가 작업을 마치면 세마포어를 "up"합니다. 이 방식은 A가 먼저 "down"하든 B가 먼저 "up"하든 상관없이 동작합니다.

예를 들면 다음과 같습니다:

```c
struct semaphore sema;

/* Thread A */
void threadA (void) {
    sema_down (&sema);
}

/* Thread B */
void threadB (void) {
    sema_up (&sema);
}

/* main function */
void main (void) {
    sema_init (&sema, 0);
    thread_create ("threadA", PRI_MIN, threadA, NULL);
    thread_create ("threadB", PRI_MIN, threadB, NULL);
}
```

이 예에서 threadA는 threadB가 `sema_up ()`를 호출할 때까지 `sema_down ()`에서 실행을 멈춥니다.

1로 초기화한 세마포어는 보통 자원 접근 제어에 사용됩니다. 코드 블록이 자원을 사용하기 전에 세마포어를 "down"하고, 자원 사용을 마친 뒤에는 이를 "up"합니다. 이런 경우에는 아래에서 설명하는 락이 더 적절할 수 있습니다.

세마포어는 1보다 큰 값으로 초기화할 수도 있습니다. 이런 경우는 드뭅니다. 세마포어는 Edsger Dijkstra가 고안했고, THE 운영체제에서 처음 사용되었습니다. Pintos의 세마포어 타입과 연산은 `include/threads/synch.h`에 선언되어 있습니다.

---

```c
struct semaphore;
```

> 세마포어를 나타냅니다.

---

```c
void sema_init (struct semaphore *sema, unsigned value);
```

> 주어진 초기값으로 sema를 새 세마포어로 초기화합니다.

---

```c
void sema_down (struct semaphore *sema);
```

sema에 대해 "down" 또는 "P" 연산을 수행합니다. 값이 양수가 될 때까지 기다린 뒤 1 감소시킵니다.

---

```c
bool sema_try_down (struct semaphore *sema);
```

> 기다리지 않고 sema에 대해 "down" 또는 "P" 연산을 시도합니다. sema를 성공적으로 감소시켰으면 true를 반환하고, sema가 이미 0이어서 기다리지 않고는 감소시킬 수 없으면 false를 반환합니다. 이 함수를 빡빡한 루프에서 호출하면 CPU 시간을 낭비하므로, 대신 `sema_down()`을 사용하거나 다른 접근법을 찾아야 합니다.

---

```c
void sema_up (struct semaphore *sema);
```

> sema에 대해 "up" 또는 "V" 연산을 수행하여 값을 증가시킵니다. sema를 기다리는 스레드가 있으면 그중 하나를 깨웁니다. 대부분의 동기화 프리미티브와 달리, `sema_up()`은 외부 인터럽트 핸들러 내부에서 호출될 수 있습니다.

세마포어는 내부적으로 인터럽트 비활성화([인터럽트 비활성화](#disabling-interrupts) 참고)와 스레드의 블로킹 및 언블로킹(`thread_block()`과 `thread_unblock()`)을 바탕으로 구현됩니다. 각 세마포어는 `lib/kernel/list.c`의 연결 리스트 구현을 사용해 대기 중인 스레드 목록을 유지합니다.

[]()</a>

## 락

락은 초기값이 1인 세마포어와 비슷합니다([세마포어](#semaphores) 참고). 락에서 "up"에 해당하는 연산은 "release", "down"에 해당하는 연산은 "acquire"라고 부릅니다.

세마포어와 비교했을 때, 락에는 한 가지 추가 제약이 있습니다. 락의 "owner", 즉 락을 획득한 스레드만 그 락을 해제할 수 있습니다. 이 제약이 문제가 된다면, 락 대신 세마포어를 사용해야 한다는 신호입니다.

Pintos의 락은 재귀적(recursive)이지 않습니다. 즉, 현재 락을 보유한 스레드가 같은 락을 다시 획득하려고 시도하면 오류입니다. 락 타입과 함수는 `include/threads/synch.h`에 선언되어 있습니다.

---

```c
struct lock;
```

> 락을 나타냅니다.

---

```c
void lock_init (struct lock *lock);
```

> lock을 새 락으로 초기화합니다. 처음에는 어떤 스레드도 이 락을 소유하지 않습니다.

---

```c
void lock_acquire (struct lock *lock);
```

> 현재 스레드가 lock을 획득합니다. 필요하다면 현재 소유자가 lock을 해제할 때까지 먼저 기다립니다.

---

```c
bool lock_try_acquire (struct lock *lock);
```

> 기다리지 않고 현재 스레드가 사용할 lock 획득을 시도합니다. 성공하면 true를, 이미 다른 스레드가 소유 중이면 false를 반환합니다. 이 함수를 빡빡한 루프에서 호출하는 것은 CPU 시간을 낭비하므로 좋지 않습니다. 대신 `lock_acquire()`를 사용하십시오.

---

```c
void lock_release (struct lock *lock);
```

> 현재 스레드가 소유하고 있어야 하는 lock을 해제합니다.

---

```c
bool lock_held_by_current_thread (const struct lock *lock):
```

> 현재 실행 중인 스레드가 lock을 소유하고 있으면 true를, 그렇지 않으면 false를 반환합니다. 임의의 스레드가 락을 소유하고 있는지 검사하는 함수는 없습니다. 그 답은 호출자가 그 결과를 바탕으로 행동하기도 전에 바뀔 수 있기 때문입니다.

[]()</a>

## 모니터

모니터는 세마포어나 락보다 더 높은 수준의 동기화 방식입니다. 모니터는 동기화 대상 데이터, 모니터 락이라고 부르는 하나의 락, 그리고 하나 이상의 조건 변수로 구성됩니다. 보호되는 데이터에 접근하기 전에 스레드는 먼저 모니터 락을 획득합니다. 그러면 그 스레드는 "모니터 안에 있다"고 말합니다. 모니터 안에 있는 동안 스레드는 보호되는 모든 데이터에 대한 제어권을 가지며, 이를 자유롭게 검사하거나 수정할 수 있습니다. 보호 데이터에 대한 접근이 끝나면 모니터 락을 해제합니다.

조건 변수는 모니터 내부의 코드가 어떤 조건이 참이 되기를 기다릴 수 있게 해 줍니다. 각 조건 변수는 "처리할 데이터가 도착했다" 또는 "사용자의 마지막 키 입력 이후 10초가 넘었다"처럼 하나의 추상적 조건에 대응합니다. 모니터 안의 코드가 어떤 조건이 참이 되기를 기다려야 할 때, 해당 조건 변수에 "wait"합니다. 그러면 락을 해제하고 조건이 signal될 때까지 기다립니다. 반대로 자신이 어떤 조건을 참으로 만들었다면, 그 조건에 대해 "signal"하여 대기자 하나를 깨우거나 "broadcast"하여 모두를 깨웁니다.

모니터의 이론적 틀은 C.A.R. Hoare가 제시했습니다. 이후 Mesa 운영체제에 관한 논문에서 그 실용적 사용법이 더 자세히 정리되었습니다. 조건 변수의 타입과 함수는 `include/threads/synch.h`에 선언되어 있습니다.

---

```c
struct condition;
```

> 조건 변수를 나타냅니다.

---

```c
void cond_init (struct condition *cond);
```

> cond를 새 조건 변수로 초기화합니다.

---

```c
void cond_wait (struct condition *cond, struct lock *lock);
```

> lock(모니터 락)을 원자적으로 해제하고, 다른 코드가 cond에 signal을 보낼 때까지 기다립니다. cond가 signal된 뒤에는 반환하기 전에 lock을 다시 획득합니다. 이 함수를 호출하기 전에 lock은 반드시 잡고 있어야 합니다. signal을 보내는 것과 wait에서 깨어나는 것은 원자적 연산이 아닙니다. 따라서 보통 `cond_wait()` 호출자는 대기가 끝난 뒤 조건을 다시 검사하고, 필요하면 다시 기다려야 합니다. 예시는 다음 절을 참고하십시오.

---

```c
void cond_signal (struct condition *cond, struct lock *lock);
```

> cond를 기다리는 스레드가 하나라도 있으면(모니터 락 lock으로 보호됨) 그중 하나를 깨웁니다. 기다리는 스레드가 없으면 아무 동작도 하지 않고 반환합니다. 이 함수를 호출하기 전에 lock은 반드시 잡고 있어야 합니다.

---

```c
void cond_broadcast (struct condition *cond, struct lock *lock);
```

> cond를 기다리는 모든 스레드를, 있다면, 깨웁니다(모니터 락 lock으로 보호됨). 이 함수를 호출하기 전에 lock은 반드시 잡고 있어야 합니다.

[]()</a>

### 모니터 예제

모니터의 고전적인 예는 하나 이상의 "producer" 스레드가 문자를 써 넣고, 하나 이상의 "consumer" 스레드가 문자를 읽어 가는 버퍼를 다루는 것입니다. 이를 구현하려면 모니터 락 외에도 `not_full`, `not_empty`라고 부를 두 개의 조건 변수가 필요합니다:

```c
char buf[BUF_SIZE];     /* Buffer. */
size_t n = 0;         /* 0 <= n <= BUF SIZE: # of characters in buffer. */
size_t head = 0;        /* buf index of next char to write (mod BUF SIZE). */
size_t tail = 0;         /* buf index of next char to read (mod BUF SIZE). */
struct lock lock;         /* Monitor lock. */
struct condition not_empty; /* Signaled when the buffer is not empty. */
struct condition not_full;     /* Signaled when the buffer is not full. */

...initialize the locks and condition variables...

void put (char ch) {
  lock_acquire (&lock);
  while (n == BUF_SIZE)    /* Can't add to buf as long as it's full. */
    cond_wait (&not_full, &lock);
  buf[head++ % BUF_SIZE] = ch;    /* Add ch to buf. */
  n++;
  cond_signal (&not_empty, &lock);    /* buf can't be empty anymore. */
  lock_release (&lock);
}

char get (void) {
  char ch;
  lock_acquire (&lock);
  while (n == 0)        /* Can't read buf as long as it's empty. */
    cond_wait (&not_empty, &lock);
  ch = buf[tail++ % BUF_SIZE];    /* Get ch from buf. */
  n--;
  cond_signal (&not_full, &lock);    /* buf can't be full anymore. */
  lock_release (&lock);
}
```

위 코드는 `BUF_SIZE`가 `SIZE_MAX + 1`을 정확히 나누어떨어지게 해야 완전히 올바릅니다. 그렇지 않으면 `head`가 0으로 되돌아가는 첫 번째 순간 실패합니다. 실제로는 `BUF_SIZE`를 보통 2의 거듭제곱으로 둡니다.

[]()</a>

## 최적화 배리어

최적화 배리어는 배리어를 기준으로 메모리 상태에 대해 컴파일러가 가정을 하지 못하게 만드는 특별한 문장입니다. 주소를 취한 적이 없는 지역 변수를 제외하면, 컴파일러는 배리어를 가로질러 변수 읽기나 쓰기를 재배치하지 않고, 변수 값이 배리어를 지나도 바뀌지 않았다고 가정하지도 않습니다. Pintos에서 `include/threads/synch.h`는 `barrier()` 매크로를 최적화 배리어로 정의합니다.

최적화 배리어를 사용하는 이유 중 하나는, 다른 스레드나 인터럽트 핸들러처럼 컴파일러가 알지 못하는 방식으로 데이터가 비동기적으로 바뀔 수 있기 때문입니다. `devices/timer.c`의 `too_many_loops()` 함수가 한 예입니다. 이 함수는 먼저 타이머 틱이 발생할 때까지 루프 안에서 바쁜 대기(busy-waiting)를 합니다:

```c
/* Wait for a timer tick. */
int64_t start = ticks;
while (ticks == start)
    barrier();
```

루프 안에 최적화 배리어가 없으면, 컴파일러는 이 루프가 끝나지 않을 것이라고 결론 내릴 수 있습니다. `start`와 `ticks`는 처음에 같고, 루프 본문 자체는 그 값을 바꾸지 않기 때문입니다. 그러면 컴파일러는 이 함수를 무한 루프로 "최적화"할 수 있는데, 이는 분명 바람직하지 않습니다.

최적화 배리어는 다른 종류의 컴파일러 최적화를 피하는 데도 사용할 수 있습니다. 역시 `devices/timer.c`에 있는 `busy_wait()` 함수가 한 예입니다. 이 함수에는 다음 루프가 들어 있습니다:

```c
while (loops-- > 0)
  barrier ();
```

이 루프의 목적은 원래 값에서 0까지 loop 수를 감소시키면서 바쁜 대기하는 것입니다. 배리어가 없으면 컴파일러는 이 루프를 완전히 지워 버릴 수 있습니다. 유용한 출력도 없고 부수 효과도 없기 때문입니다. 배리어는 컴파일러가 루프 본문에 중요한 효과가 있다고 가장하도록 강제합니다.

마지막으로, 최적화 배리어는 메모리 읽기나 쓰기 순서를 강제하는 데 사용할 수 있습니다. 예를 들어 타이머 인터럽트가 발생할 때마다 전역 변수 `timer_put_char`에 들어 있는 문자를 콘솔에 출력하되, 전역 불리언(Boolean) 변수 `timer_do_put`이 true일 때만 출력하는 "기능"을 추가했다고 가정해 봅시다. 그렇다면 `x`가 출력되도록 설정하는 가장 좋은 방법은 다음처럼 최적화 배리어를 사용하는 것입니다:

```c
timer_put_char = 'x';
barrier ();
timer_do_put = true;
```

배리어가 없으면 이 코드는 버그가 있습니다. 컴파일러는 같은 순서를 유지해야 할 이유를 보지 못하면 연산 순서를 자유롭게 바꿀 수 있기 때문입니다. 이 경우 컴파일러는 대입 순서가 중요하다는 사실을 모르므로, 최적화기는 두 연산의 순서를 바꿔도 됩니다. 실제로 그렇게 할지는 알 수 없고, 컴파일러에 다른 최적화 플래그를 주거나 다른 버전을 쓰면 동작이 달라질 수도 있습니다.

또 다른 해결책은 대입문 주변에서 인터럽트를 비활성화하는 것입니다. 이는 재배치를 막지는 못하지만, 인터럽트 핸들러가 두 대입문 사이에 끼어드는 것은 막아 줍니다. 대신 인터럽트를 비활성화했다가 다시 활성화하는 추가 런타임 비용이 듭니다:

```c
enum intr_level old_level = intr_disable ();
timer_put_char = 'x';
timer_do_put = true;
intr_set_level (old_level);
```

두 번째 해결책은 `timer_put_char`와 `timer_do_put`의 선언에 `volatile`을 붙이는 것입니다. 이 키워드는 해당 변수가 외부에서 관측 가능하다고 컴파일러에 알려 주어 최적화 재량을 제한합니다. 하지만 `volatile`의 의미론은 잘 정의되어 있지 않으므로 일반적인 해결책으로는 좋지 않습니다. 기본 Pintos 코드는 `volatile`을 전혀 사용하지 않습니다.

다음 방법은 해결책이 아닙니다. 락은 인터럽트를 막지도 못하고, 락을 잡은 구역 내부의 코드를 컴파일러가 재배치하는 것도 막지 못하기 때문입니다:

```c
lock_acquire (&timer_lock);        /* INCORRECT CODE */
timer_put_char = 'x';
timer_do_put = true;
lock_release (&timer_lock);
```

컴파일러는 외부에서 정의된 함수, 즉 다른 소스 파일에 정의된 함수의 호출을 제한적인 형태의 최적화 배리어로 취급합니다. 구체적으로, 컴파일러는 외부 정의 함수가 정적 또는 동적으로 할당된 어떤 데이터든, 그리고 주소를 취한 어떤 지역 변수든 접근할 수 있다고 가정합니다. 이런 이유로 명시적 배리어를 생략해도 되는 경우가 많습니다. Pintos에 명시적 배리어가 많지 않은 이유 중 하나도 여기에 있습니다.

같은 소스 파일 안에 정의된 함수나, 그 소스 파일이 포함하는 헤더에 정의된 함수는 최적화 배리어로 믿을 수 없습니다. 함수 정의보다 앞에서 그 함수를 호출하는 경우에도 마찬가지입니다. 컴파일러는 최적화를 수행하기 전에 전체 소스 파일을 읽고 파싱할 수 있기 때문입니다.
