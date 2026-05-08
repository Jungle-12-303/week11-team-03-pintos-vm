# 04_virtual_address

# 가상 주소

64비트 가상 주소는 다음과 같이 구성됩니다:

```
63          48 47            39 38            30 29            21 20         12 11         0
+-------------+----------------+----------------+----------------+-------------+------------+
| Sign Extend |    Page-Map    | Page-Directory | Page-directory |  Page-Table |  Physical  |
|             | Level-4 Offset |    Pointer     |     Offset     |   Offset    |   Offset   |
+-------------+----------------+----------------+----------------+-------------+------------+
              |                |                |                |             |            |
              +------- 9 ------+------- 9 ------+------- 9 ------+----- 9 -----+---- 12 ----+
                                          Virtual Address
```

헤더 `include/threads/vaddr.h`와 `include/threads/mmu.h`에는 가상 주소를 다루기 위한 다음 함수와 매크로가 정의되어 있습니다:

---

```c
#define PGSHIFT { /* Omit details */ }
#define PGBITS { /* Omit details */ }
```

> 가상 주소 오프셋 부분의 비트 인덱스(0)와 비트 수(12)를 각각 나타냅니다.

---

```c
#define PGMASK { /* Omit details */ }
```

> 페이지 오프셋에 해당하는 비트만 1이고 나머지는 0인 비트 마스크(0xfff)입니다.

---

```c
#define PGSIZE { /* Omit details */ }
```

> 페이지 크기(바이트 단위, 4,096)입니다.

---

```c
#define pg_ofs(va) { /* Omit details */ }
```

> 가상 주소 `va`의 페이지 오프셋을 추출해 반환합니다.

---

```c
#define pg_no(va) { /* Omit details */ }
```

> 가상 주소 `va`의 페이지 번호를 추출해 반환합니다.

---

```c
#define pg_round_down(va) { /* Omit details */ }
```

> `va`가 속한 가상 페이지의 시작 주소를 반환합니다. 즉, `va`에서 페이지 오프셋을 0으로 만든 값입니다.

---

```c
#define pg_round_up(va) { /* Omit details */ }
```

> `va`를 가장 가까운 페이지 경계까지 올림한 값을 반환합니다.

Pintos의 가상 메모리는 사용자 가상 메모리와 커널 가상 메모리의 두 영역으로 나뉩니다([가상 메모리 레이아웃](../03_project2_user_programs/01_introduction.md) 참고).

이 둘의 경계는 `KERN_BASE`입니다:

---

```c
#define KERN_BASE { /* Omit details */ }
```

> 커널 가상 메모리의 기준 주소입니다. 기본값은 0x8004000000입니다.
> 사용자 가상 메모리는 가상 주소 0부터 `KERN_BASE` 직전까지입니다.
> 커널 가상 메모리는 가상 주소 공간의 나머지 부분을 차지합니다.

---

```c
#define is_user_vaddr(vaddr) { /* Omit details */ }
#define is_kernel_vaddr(vaddr) { /* Omit details */ }
```

> `va`가 각각 사용자 또는 커널 가상 주소이면 true를, 그렇지 않으면 false를 반환합니다.

---

x86-64는 물리 주소만으로 메모리에 직접 접근하는 방법을 제공하지 않습니다. 운영체제 커널에서는 이런 기능이 자주 필요하므로, Pintos는 커널 가상 메모리를 물리 메모리에 1:1로 매핑하는 방식으로 이를 우회합니다. 즉, 가상 주소 `KERN_BASE`는 물리 주소 0에 접근하고, 가상 주소 `KERN_BASE + 0x1234`는 물리 주소 0x1234에 접근하며, 이런 식으로 시스템의 물리 메모리 크기까지 이어집니다. 따라서 물리 주소에 `KERN_BASE`를 더하면 해당 주소에 접근하는 커널 가상 주소를 얻을 수 있고, 반대로 커널 가상 주소에서 `KERN_BASE`를 빼면 대응하는 물리 주소를 얻을 수 있습니다.

헤더 `include/threads/vaddr.h`는 이 변환을 수행하는 함수 두 개를 제공합니다:

---

```c
#define ptov(paddr) { /* Omit details */ }
```

> 물리 주소 `pa`에 대응하는 커널 가상 주소를 반환합니다. `pa`는 0 이상이어야 하며 물리 메모리의 총 바이트 수 이하여야 합니다.

---

```c
#define vtop(vaddr) { /* Omit details */ }
```

> `va`에 대응하는 물리 주소를 반환합니다. `va`는 반드시 커널 가상 주소여야 합니다.

헤더 `include/threads/mmu.h`는 페이지 테이블 연산을 제공합니다:

---

```c
#define is_user_pte(pte) { /* Omit details */ }
#define is_kern_pte(pte) { /* Omit details */ }
```

> 페이지 테이블 엔트리(PTE)가 각각 사용자 또는 커널 소유인지 확인합니다.

---

```c
#define is_writable(pte) { /* Omit details */ }
```

> 페이지 테이블 엔트리(PTE)가 가리키는 가상 주소가 쓰기 가능한지 확인합니다.

---

```c
typedef bool pte_for_each_func (uint64_t *pte, void *va, void *aux);
bool pml4_for_each (uint64_t *pml4, pte_for_each_func *func, void *aux);
```

> PML4 아래의 각 유효한 엔트리에 대해 보조 값 AUX와 함께 FUNC를 적용합니다.
> VA는 해당 엔트리의 가상 주소를 나타냅니다. `pte_for_each_func`가 false를 반환하면 순회를 중단하고 false를 반환합니다.

아래는 `pml4_for_each`에 넘길 수 있는 `func` 예시입니다:

```c
static bool
stat_page (uint64_t *pte, void *va,  void *aux) {
        if (is_user_vaddr (va))
                printf ("user page: %llx\n", va);
        if (is_writable (va))
                printf ("writable page: %llx\n", va);
        return true;
}
```
