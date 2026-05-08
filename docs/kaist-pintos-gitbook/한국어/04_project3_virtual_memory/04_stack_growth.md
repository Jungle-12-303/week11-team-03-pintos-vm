# 04_stack_growth

### 스택 확장

project 2에서는 스택이 `USER_STACK`에서 시작하는 단일 페이지였고, 프로그램의
실행도 이 크기에 제한되었습니다. 이제는 스택이 현재 크기를 넘어서 커지면,
필요에 따라 추가 페이지를 할당합니다.

추가 페이지는 그것이 스택 접근으로 "보일" 때만 할당하세요. 스택 접근과 그
밖의 접근을 구분하려는 heuristic을 설계해야 합니다.

사용자 프로그램이 stack pointer 아래의 스택에 기록하면 버그가 될 수 있습니다.
실제 OS는 프로세스에 "signal"을 전달하기 위해 언제든지 인터럽트할 수 있고,
그 과정에서 스택의 데이터를 수정할 수 있기 때문입니다. 하지만 x86-64의
PUSH 명령은 stack pointer를 조정하기 전에 접근 권한을 검사하므로, stack
pointer보다 8바이트 아래에서 페이지 폴트를 일으킬 수 있습니다.

사용자 프로그램의 현재 stack pointer 값을 얻을 수 있어야 합니다. 시스템 콜
안이나 사용자 프로그램이 일으킨 페이지 폴트 안에서는 각각
`syscall_handler()`나 `page_fault()`에 전달되는 `struct intr_frame`의
`rsp` 멤버에서 이 값을 얻을 수 있습니다. 유효하지 않은 메모리 접근을 감지하는
데 페이지 폴트에 의존한다면, 커널에서 페이지 폴트가 발생하는 또 다른 경우도
처리해야 합니다. 프로세서는 예외가 사용자 모드에서 커널 모드로 전환될 때만
stack pointer를 저장하므로, `page_fault()`에 전달된 `struct intr_frame`
에서 `rsp`를 읽으면 사용자 stack pointer가 아니라 정의되지 않은 값을 얻게
됩니다. 따라서 사용자 모드에서 커널 모드로 처음 전환될 때 `rsp`를
`struct thread`에 저장하는 등의 다른 방법을 마련해야 합니다.

**스택 확장 기능을 구현하세요.** 
이를 위해 먼저 `vm/vm.c`의 `vm_try_handle_fault`를 수정해 스택 확장을
식별해야 합니다. 스택 확장이라고 판단되면 `vm/vm.c`의 `vm_stack_growth`를
호출해 스택을 확장하세요. 그리고 `vm_stack_growth`를 구현하세요.

---

```c
bool vm_try_handle_fault (struct intr_frame *f, void *addr,
    bool user, bool write, bool not_present);
```

> 이 함수는 페이지 폴트 예외를 처리하는 동안 `userprog/exception.c`의
> `page_fault`에서 호출됩니다. 이 함수 안에서는 현재 페이지 폴트가 스택
> 확장으로 처리할 수 있는 유효한 경우인지 확인해야 합니다. 스택 확장으로
> 처리할 수 있다고 확신했다면, 폴트가 발생한 주소를 사용해 `vm_stack_growth`
> 를 호출하세요.

---

```c
void vm_stack_growth (void *addr);
```

> 하나 이상의 익명 페이지를 할당해 스택 크기를 늘리고, 그 결과 `addr`이 더
> 이상 페이지 폴트를 일으키는 주소가 아니게 만드세요. 할당할 때는 반드시
> `addr`을 `PGSIZE` 단위로 round down 하세요.

대부분의 OS는 스택 크기에 어떤 절대적인 제한을 둡니다. 많은 Unix 계열
시스템에서는 `ulimit` 명령을 통해 이 제한을 사용자가 조정할 수 있습니다.
많은 GNU/Linux 시스템의 기본 제한은 8 MB입니다. 이번 프로젝트에서는 스택
크기를 최대 1MB로 제한해야 합니다.

이제 모든 스택 확장 테스트 케이스를 통과해야 합니다.
