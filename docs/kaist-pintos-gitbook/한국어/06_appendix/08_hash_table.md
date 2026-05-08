# 08_hash_table

# 해시 테이블

Pintos는 `lib/kernel/hash.c`에 해시 테이블 자료구조를 제공합니다. 이를 사용하려면 `#include <hash.h>`로 헤더 파일인 `lib/kernel/hash.h`를 포함해야 합니다. Pintos가 제공하는 코드 중 해시 테이블을 사용하는 부분은 없으므로, 여러분은 원하는 대로 이를 그대로 사용하거나, 목적에 맞게 구현을 수정하거나, 아예 사용하지 않아도 됩니다.

가상 메모리 프로젝트 구현의 대부분은 페이지를 프레임으로 변환하는 데 해시 테이블을 사용합니다. 해시 테이블은 그 밖의 다른 용도로도 쓸 수 있을 것입니다.

## 자료형

해시 테이블은 `struct hash`로 표현됩니다.

---

```c
struct hash;
```

> 전체 해시 테이블을 나타냅니다. `struct hash`의 실제 멤버는 외부에 드러나지 않습니다. 즉, 해시 테이블을 사용하는 코드는 `struct hash`의 멤버에 직접 접근해서는 안 되며, 그럴 필요도 없습니다. 대신 해시 테이블 함수와 매크로를 사용하십시오.

해시 테이블이 다루는 원소는 `struct hash_elem` 타입입니다.

---

```c
struct hash_elem;
```

> 해시 테이블에 넣고 싶은 구조체 안에 `struct hash_elem` 멤버를 포함시키십시오. `struct hash`와 마찬가지로 `struct hash_elem`도 외부에 드러나지 않습니다. 해시 테이블 원소를 다루는 모든 함수는 실제 원소 타입 포인터가 아니라 `struct hash_elem`에 대한 포인터를 받고 반환합니다.

해시 테이블의 실제 원소로부터 `struct hash_elem`을 얻어야 할 때도 많고, 그 반대도 자주 필요합니다. 해시 테이블의 실제 원소가 주어졌다면 `&` 연산자로 그 안의 `struct hash_elem` 포인터를 얻을 수 있습니다. 반대 방향으로는 `hash_entry()` 매크로를 사용하십시오.

---

```c
#define hash_entry (elem, type, member) { /* Omit details */ }
```

> `struct hash_elem`에 대한 포인터인 elem이 포함된 구조체에 대한 포인터를 반환합니다. 이를 위해서는 elem이 들어 있는 구조체 이름인 type과, elem이 가리키는 type 안의 멤버 이름인 member를 제공해야 합니다.
>
> 예를 들어 `h`가 `h_elem`이라는 이름의 `struct hash_elem` 타입 멤버를 가리키는 `struct hash_elem *` 변수라고 가정해 보겠습니다. 그러면 `hash_entry` (`h, struct thread, h_elem`)는 `h`가 속한 `struct thread`의 주소를 얻습니다.

각 해시 테이블 원소는 키를 포함해야 합니다. 키는 원소를 식별하고 서로 구별하는 데이터이며, 해시 테이블 안의 원소들 사이에서 유일해야 합니다. (원소에는 유일할 필요가 없는 비키 데이터가 함께 들어 있을 수도 있습니다.) 원소가 해시 테이블 안에 들어 있는 동안에는 그 키 데이터를 바꿔서는 안 됩니다. 꼭 바꿔야 한다면, 먼저 원소를 해시 테이블에서 제거하고 키를 수정한 뒤 다시 삽입해야 합니다.

각 해시 테이블마다 키를 다루는 함수 두 개를 직접 작성해야 합니다. 하나는 해시 함수이고, 다른 하나는 비교 함수입니다. 이 함수들은 다음 프로토타입과 일치해야 합니다:

---

```c
typedef unsigned hash_hash_func (const struct hash_elem *e, void *aux);
```

> 원소 데이터의 해시값을 `unsigned int` 범위 안의 값으로 반환합니다. 원소의 해시값은 그 원소 키의 의사난수 함수여야 합니다. 해시값은 원소 안의 비키 데이터나, 키를 제외한 비상수 데이터에 의존해서는 안 됩니다. Pintos는 해시 함수를 작성할 때 기반으로 삼을 수 있는 다음 함수들을 제공합니다.
>
> `unsigned hash_bytes (const void *buf, size t *size)`
>
>> buf에서 시작하는 size 바이트의 해시값을 반환합니다. 구현은 32비트 워드용 범용 Fowler-Noll-Vo hash (`http://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash`)입니다.
>>
>
> `unsigned hash_string (const char *s)`
>
>> null-terminated 문자열 `s`의 해시값을 반환합니다.
>>
>
> `unsigned hash_int (int i)`
>
>> 정수 i의 해시값을 반환합니다.
>>
>
> 키가 적절한 타입의 단일 데이터 하나라면, 이 함수들 중 하나의 반환값을 그대로 해시 함수 결과로 쓰는 것이 합리적입니다. 여러 데이터 조각으로 이루어진 키라면, 예를 들어 '^' (exclusive or) 연산자를 사용해 이 함수들을 여러 번 호출한 결과를 조합할 수 있습니다. 물론 이 함수들을 완전히 무시하고 처음부터 직접 해시 함수를 작성해도 되지만, 여러분의 목표는 해시 함수를 설계하는 것이 아니라 운영체제 커널을 만드는 일이라는 점을 기억하십시오. aux에 대한 설명은 [보조 데이터](#보조-데이터) 절을 참고하십시오.
>
> `bool hash_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux)`
>
>> 원소 a와 b에 저장된 키를 비교합니다. a가 b보다 작으면 true를, a가 b보다 크거나 같으면 false를 반환합니다.
>> 두 원소가 같다고 비교된다면, 두 원소의 해시값도 같아야 합니다.
>>
>
> aux에 대한 설명은 [보조 데이터](#보조-데이터) 절을 참고하십시오.
> 해시 함수와 비교 함수의 예시는 [해시 테이블 예제](#해시-테이블-예제) 절을 참고하십시오.
> 몇몇 함수는 세 번째 종류의 함수에 대한 포인터를 인자로 받습니다:
>
> `void hash_action_func (struct hash_elem *element, void *aux)`
>
>> 호출자가 선택한 어떤 동작을 element에 수행합니다.
>> aux에 대한 설명은 [보조 데이터](#보조-데이터) 절을 참고하십시오.
>>

## 기본 함수

이 함수들은 해시 테이블을 만들고, 파괴하고, 상태를 살펴봅니다.

---

```c
bool hash_init (struct hash *hash, hash_hash_func *hash_func,
                    hash_less_func *less_func, void *aux)
```

> hash를 해시 함수 `hash_func`, 비교 함수 `less_func`, 보조 데이터 `aux`를 사용하는 해시 테이블로 초기화합니다. 성공하면 true를, 실패하면 false를 반환합니다. `hash_init()`는 `malloc()`을 호출하므로 메모리를 할당할 수 없으면 실패합니다. aux에 대한 설명은 Hash Auxiliary Data를 참고하십시오. 대부분의 경우 aux는 널 포인터입니다.

---

```c
void hash_clear (struct hash *hash, hash_action_func *action)
```

> 이전에 `hash_init()`으로 초기화된 hash에서 모든 원소를 제거합니다.
> action이 non-null이면 해시 테이블의 각 원소마다 한 번씩 호출됩니다. 이를 통해 호출자는 원소가 사용하던 메모리나 기타 자원을 해제할 기회를 얻습니다. 예를 들어 해시 테이블 원소를 `malloc()`으로 동적으로 할당했다면 action에서 원소를 `free()`할 수 있습니다. 이는 `hash_clear()`가 어떤 원소에 대해 action을 호출한 뒤에는 그 해시 원소의 메모리에 더 이상 접근하지 않기 때문에 안전합니다. 하지만 action은 `hash_insert()`나 `hash_delete()`처럼 해시 테이블을 수정할 수 있는 어떤 함수도 호출해서는 안 됩니다.

---

```c
void hash_destroy (struct hash *hash, hash_action_func *action);
```

> action이 non-null이면, `hash_clear()`를 호출할 때와 같은 의미로 hash의 각 원소에 대해 action을 호출합니다. 그런 다음 hash가 사용하던 메모리를 해제합니다. 이후에는 그 사이에 `hash_init()`을 다시 호출하지 않는 한 hash를 어떤 해시 테이블 함수에도 넘겨서는 안 됩니다.

---

```c
size_t hash_size (struct hash *hash);
```

> 현재 hash에 저장된 원소 수를 반환합니다.

---

```c
bool hash_empty (struct hash *hash);
```

> 현재 hash에 원소가 하나도 없으면 true를, 하나 이상 있으면 false를 반환합니다.

## 검색 함수

이 함수들은 각각 해시 테이블에서 주어진 원소와 같다고 비교되는 원소를 찾습니다. 검색 성공 여부에 따라 해시 테이블에 새 원소를 삽입하는 등의 동작을 수행하거나, 단순히 검색 결과를 반환합니다.

---

```c
struct hash_elem *hash_insert (struct hash *hash, struct hash elem *element);
```

> hash에서 element와 같다고 비교되는 원소를 찾습니다. 그런 원소가 없으면 element를 hash에 삽입하고 널 포인터를 반환합니다. 이미 같다고 비교되는 원소가 있으면 hash를 수정하지 않고 그 원소를 반환합니다.

---

```c
struct hash_elem *hash_replace (struct hash *hash, struct hash elem *element);
```

> hash에 element를 삽입합니다. 이미 hash 안에 element와 같다고 비교되는 원소가 있으면 제거합니다.
> 제거된 원소를 반환하며, hash 안에 element와 같은 원소가 없었다면 널 포인터를 반환합니다.
>
> 반환된 원소와 관련된 자원 해제 책임은 필요에 따라 호출자에게 있습니다. 예를 들어 해시 테이블 원소를 `malloc()`으로 동적으로 할당했다면, 더 이상 필요 없어진 뒤 호출자가 그 원소를 `free()`해야 합니다.

다음 함수들에 넘기는 element는 해싱과 비교에만 사용됩니다. 실제로 해시 테이블에 삽입되지는 않습니다. 따라서 element 안에서는 키 데이터만 초기화하면 되고, 다른 데이터는 사용되지 않습니다. 종종 원소 타입의 인스턴스를 지역 변수로 선언하고 키 데이터만 초기화한 뒤, 그 `struct hash_elem`의 주소를 `hash_find()`나 `hash_delete()`에 넘기는 것이 적절합니다. 예시는 Hash Table Example을 참고하십시오. (큰 구조체는 지역 변수로 할당해서는 안 됩니다. 자세한 내용은 [struct thread](01_threads.md)를 참고하십시오.)

---

```c
struct hash_elem *hash_find (struct hash *hash, struct hash elem *element);
```

> hash에서 element와 같다고 비교되는 원소를 찾습니다. 찾으면 그 원소를 반환하고, 없으면 널 포인터를 반환합니다.

---

```c
struct hash_elem *hash_delete (struct hash *hash, struct hash elem *element);
```

> hash에서 element와 같다고 비교되는 원소를 찾습니다. 찾으면 hash에서 제거한 뒤 반환합니다. 그렇지 않으면 널 포인터를 반환하고 hash는 그대로 유지됩니다.
>
> 반환된 원소와 관련된 자원 해제 책임은 필요에 따라 호출자에게 있습니다. 예를 들어 해시 테이블 원소를 `malloc()`으로 동적으로 할당했다면, 더 이상 필요 없어진 뒤 호출자가 그 원소를 `free()`해야 합니다.

## 순회 함수

이 함수들은 해시 테이블의 원소들을 순회할 수 있게 해 줍니다. 두 가지 인터페이스가 제공됩니다. 첫 번째는 각 원소에 대해 동작할 `hash_action_func`를 작성해서 넘기는 방식입니다.

---

```c
void hash_apply (struct hash *hash, hash action func *action);
```

> hash 안의 각 원소에 대해 action을 임의 순서로 한 번씩 호출합니다. action은 `hash_insert()`나 `hash_delete()`처럼 해시 테이블을 수정할 수 있는 어떤 함수도 호출해서는 안 됩니다. action은 원소의 키 데이터는 수정해서는 안 되지만, 그 밖의 데이터는 수정해도 됩니다.

두 번째 인터페이스는 "iterator" 자료형에 기반합니다. 관용적으로는 다음과 같이 iterator를 사용합니다:

---

```c
struct hash_iterator i;
hash_first (&i, h);
while (hash_next (&i)) {
    struct foo *f = hash_entry (hash_cur (&i), struct foo, elem);
    . . . do something with f . . .
}
```

---

```c
struct hash_iterator;
```

> 해시 테이블 안의 한 위치를 나타냅니다. `hash_insert()`나 `hash_delete()`처럼 해시 테이블을 수정할 수 있는 함수를 호출하면, 그 해시 테이블 안의 모든 iterator가 무효화됩니다.
>
> `struct hash`와 `struct hash_elem`과 마찬가지로, 이 타입 역시 외부에 드러나지 않습니다.

---

```c
void hash_first (struct hash iterator *iterator, struct hash *hash);
```

> iterator를 hash의 첫 번째 원소 바로 앞 위치로 초기화합니다.

```c
struct hash_elem *hash_next (struct hash iterator *iterator);
```

> iterator를 hash의 다음 원소로 전진시키고, 그 원소를 반환합니다.
> 남은 원소가 없으면 널 포인터를 반환합니다. `hash_next()`가 iterator에 대해 null을 반환한 뒤 다시 호출하면 정의되지 않은 동작입니다.

---

```c
struct hash_elem *hash_cur (struct hash iterator *iterator);
```

> iterator에 대해 `hash_next()`가 가장 최근에 반환한 값을 반환합니다. `hash_first()`를 iterator에 호출한 뒤 첫 번째 `hash_next()`가 아직 호출되지 않았다면 정의되지 않은 동작입니다.

## 해시 테이블 예시

해시 테이블에 넣고 싶은 `struct page`라는 구조체가 있다고 가정해 보겠습니다. 먼저 `struct page`에 `struct hash_elem` 멤버가 들어가도록 정의합니다:

```c
struct page {
    struct hash_elem hash_elem; /* Hash table element. */
    void *addr; /* Virtual address. */
    /* . . . other members. . . */
};
```

키로 addr를 사용해서 해시 함수와 비교 함수를 작성합니다. 포인터는 그 바이트 표현을 기준으로 해시할 수 있고, 포인터 비교에는 `<` 연산자를 그대로 사용해도 괜찮습니다:

```c
/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->addr, sizeof p->addr);
}
```

```c
/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->addr < b->addr;
}
```

(이 함수 프로토타입에서 `UNUSED`를 사용한 것은 aux가 사용되지 않는다는 경고를 억제하기 위해서입니다. `UNUSED`에 대한 정보는 Function and Parameter Attributes를 참고하십시오. aux에 대한 설명은 Hash Auxiliary Data를 참고하십시오.)

그런 다음 다음과 같이 해시 테이블을 만들 수 있습니다:

```c
struct hash pages;
hash_init (&pages, page_hash, page_less, NULL);
```

이제 이렇게 만든 해시 테이블을 조작할 수 있습니다. `p`가 `struct page`에 대한 포인터라면, 다음과 같이 해시 테이블에 삽입할 수 있습니다:

```c
hash_insert (&pages, &p->hash_elem);
```

이미 `pages` 안에 같은 addr를 가진 페이지가 있을 가능성이 있다면, `hash_insert()`의 반환값을 확인해야 합니다.

해시 테이블에서 원소를 찾으려면 `hash_find()`를 사용하십시오. `hash_find()`는 비교 대상 원소를 인자로 받기 때문에 약간의 준비가 필요합니다. 아래 함수는 `pages`가 파일 스코프에 정의되어 있다고 가정하고, 가상 주소를 기준으로 페이지를 찾아 반환합니다:

```c
/* Returns the page containing the given virtual address, or a null pointer if no such page exists. */
struct page *
page_lookup (const void *address) {
  struct page p;
  struct hash_elem *e;

  p.addr = address;
  e = hash_find (&pages, &p.hash_elem);
  return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}
```

여기서는 `struct page`가 비교적 작은 구조체라고 가정하고 지역 변수로 할당했습니다. 큰 구조체는 지역 변수로 할당해서는 안 됩니다.

비슷한 함수로 `hash_delete()`를 사용해 주소를 기준으로 페이지를 삭제할 수도 있습니다.

## 보조 데이터

위 예시처럼 단순한 경우에는 aux 매개변수가 필요 없습니다. 이런 경우에는 `hash_init()`에 aux로 널 포인터를 넘기고, 해시 함수와 비교 함수로 전달되는 값은 무시하면 됩니다. (aux 매개변수를 사용하지 않으면 컴파일러 경고가 나오지만, 예시처럼 `UNUSED` 매크로로 그 경고를 끌 수 있고, 아니면 그냥 무시해도 됩니다.)

***aux***는 해시 테이블 데이터의 어떤 속성이 상수이면서 해싱이나 비교에 필요하지만, 그 속성이 데이터 항목 자체에는 저장되어 있지 않을 때 유용합니다. 예를 들어 해시 테이블의 항목이 고정 길이 문자열인데, 항목 자체에는 그 고정 길이가 드러나지 않는다면 그 길이를 aux 매개변수로 넘길 수 있습니다.

## 동기화

해시 테이블은 내부 동기화를 수행하지 않습니다. 해시 테이블 함수 호출을 동기화하는 책임은 호출자에게 있습니다. 일반적으로 `hash_find()`나 `hash_next()`처럼 해시 테이블을 살펴보기만 하고 수정하지 않는 함수는 여러 개가 동시에 실행될 수 있습니다. 하지만 이런 함수는 `hash_insert()`나 `hash_delete()`처럼 주어진 해시 테이블을 수정할 수 있는 함수와 동시에 안전하게 실행될 수 없고, 같은 해시 테이블을 수정할 수 있는 함수 둘 이상도 동시에 안전하게 실행될 수 없습니다.

해시 테이블 원소 안의 데이터 접근을 동기화하는 책임 역시 호출자에게 있습니다. 이 데이터를 어떻게 동기화할지는 다른 자료구조와 마찬가지로, 그 데이터가 어떻게 설계되고 조직되어 있는지에 따라 달라집니다.
