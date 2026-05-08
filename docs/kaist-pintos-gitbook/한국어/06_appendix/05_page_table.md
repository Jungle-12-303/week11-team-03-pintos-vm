# 05_page_table

[]()</a>

# 페이지 테이블

`threads/mmu.c`의 코드는 x86_64 하드웨어 페이지 테이블에 대한 추상 인터페이스입니다. Pintos의 페이지 테이블(프로젝트에서 여러분이 사용하게 될 바로 그 페이지 테이블)은 Intel 프로세서 문서에서 Page-Map-Level-4의 약자로 부르는 `pml4`라고 합니다. 이 테이블이 4단계 구조이기 때문입니다. 페이지 테이블 인터페이스는 내부 구조에 접근하기 편리하도록 페이지 테이블을 나타낼 때 `uint64_t *`를 사용합니다. 아래 절에서는 페이지 테이블 인터페이스와 내부 구조를 설명합니다.

[]()</a>

## 생성, 파기, 활성화

이 함수들은 페이지 테이블을 생성하고, 파기하고, 활성화합니다. 기본 Pintos 코드는 필요한 곳에서 이미 이 함수들을 호출하므로, 여러분이 직접 호출할 일은 없을 것입니다.

---

```c
uint64_t * pml4_create (void);
```

> 새 페이지 테이블을 생성해 반환합니다. 새 페이지 테이블에는 Pintos의 일반적인 커널 가상 페이지 매핑은 들어 있지만, 사용자 가상 매핑은 들어 있지 않습니다. 메모리를 얻을 수 없으면 널 포인터를 반환합니다.

---

```c
void pml4_destroy (uint64_t *pml4);
```

> pml4가 보유한 모든 리소스를 해제합니다. 페이지 테이블 자체와 그것이 매핑하는 프레임도 포함됩니다. 페이지 테이블의 모든 수준에 있는 리소스를 해제하기 위해 `pdpe_destroy`, `pgdir_destory`, `pt_destroy`를 *재귀적으로* 호출합니다.

---

```c
void pml4_activate (uint64 t *pml4)
```

> pml4를 활성화합니다. 활성 페이지 테이블은 CPU가 메모리 참조를 변환할 때 사용하는 페이지 테이블입니다.

[]()</a>

## 검사와 갱신

이 함수들은 페이지 테이블이 캡슐화한 페이지와 프레임 사이의 매핑을 살펴보거나 갱신합니다. 실행 중인 프로세스와 일시 중단된 프로세스의 페이지 테이블, 즉 활성 페이지 테이블과 비활성 페이지 테이블 모두에서 동작하며, 필요하면 TLB를 flush합니다.

---

```c
bool pml4_set_page (uint64_t *pml4, void *upage, void *kpage, bool rw);
```

> 사용자 페이지 upage를 커널 가상 주소 kpage가 식별하는 프레임에 매핑하도록 pml4에 추가합니다. rw가 true이면 이 페이지는 읽기/쓰기로 매핑되고, 그렇지 않으면 읽기 전용으로 매핑됩니다. 사용자 페이지 upage는 pml4에 이미 매핑되어 있어서는 안 됩니다. 커널 페이지 kpage는 `palloc_get_page(PAL_USER)`로 사용자 풀에서 얻은 커널 가상 주소여야 합니다. 성공하면 true를, 실패하면 false를 반환합니다. 페이지 테이블에 추가로 필요한 메모리를 얻지 못하면 실패합니다.

---

```c
void * pml4_get_page (uint64_t *pml4, const void *uaddr);
```

> pml4에서 uaddr에 매핑된 프레임을 조회합니다. uaddr이 매핑되어 있으면 해당 프레임의 커널 가상 주소를 반환하고, 매핑되어 있지 않으면 널 포인터를 반환합니다.

---

```c
void pml4_clear_page (uint64_t *pml4, void *upage);
```

> pml4에서 페이지를 "not present"로 표시합니다. 이후 이 페이지에 접근하면 fault가 발생합니다. 페이지 테이블 엔트리의 다른 비트는 유지되므로, accessed 비트와 dirty 비트(다음 절 참고)를 검사할 수 있습니다. 이 함수는 해당 페이지가 매핑되어 있지 않으면 아무 효과도 없습니다.

[]()</a>

## Accessed 비트와 Dirty 비트

x86_64 하드웨어는 각 페이지의 page table entry(PTE)에 있는 두 비트를 통해 페이지 교체 알고리즘 구현을 일부 지원합니다. 페이지에 대해 읽기 또는 쓰기가 일어날 때마다 CPU는 그 페이지의 PTE에서 accessed 비트를 1로 설정하고, 쓰기가 일어날 때마다 dirty 비트를 1로 설정합니다. CPU는 이 비트들을 절대 0으로 되돌리지 않지만, 운영체제는 그렇게 할 수 있습니다. 이 비트를 올바르게 해석하려면 *별칭(alias)* , 즉 같은 프레임을 가리키는 두 개 이상의 페이지를 이해해야 합니다. 별칭이 걸린 프레임에 접근하면 accessed 비트와 dirty 비트는 페이지 테이블 엔트리 하나에만 갱신됩니다(접근에 사용된 페이지의 엔트리). 다른 별칭들의 accessed 비트와 dirty 비트는 갱신되지 않습니다.

---

```c
bool pml4_is_dirty (uint64_t *pml4, const void *vpage);
bool pml4_is_accessed (uint64_t *pml4, const void *vpage);
```

> pml4가 vpage에 대한 페이지 테이블 엔트리를 가지고 있고 그 엔트리가 dirty(또는 accessed)로 표시되어 있으면 true를 반환합니다. 그렇지 않으면 false를 반환합니다.

---

```c
void pml4_set_dirty (uint64_t *pml4, const void *vpage, bool dirty);
void pml4_set_accessed (uint64_t *pml4, const void *vpage, bool accessed);
```

> pml4에 해당 페이지에 대한 페이지 테이블 엔트리가 있으면, 그 dirty 비트(또는 accessed 비트)를 주어진 값으로 설정합니다.
