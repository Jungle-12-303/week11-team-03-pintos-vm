/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include <hash.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "userprog/process.h"
#include <list.h>
#include <string.h>

#define USER_STACK_MAX_SIZE (1 << 20)

static struct list frame_table;
static struct lock frame_lock;

static void vm_frame_table_init (void);

// (참고) stack growth가 동작하면 fault 주소가 속한 4KB page 하나를 새 anonymous page로 만들고, 그 frame을 0으로 채워 초기화한다.

/* stack growth가 필요한 경우:
    사용자 코드가 어떤 stack 주소에 접근했는데,
    그 주소가 아직 매핑되어 있지 않아서 page fault가 났고,
    그 주소가 정상적인 stack 접근처럼 보일 때,
    즉, rsp가 감소하다가 아직 없는 더 아래쪽 페이지에 닿을 때 새 페이지를 만듭니다.
    또는 큰 stack object처럼 rsp를 크게 낮춘 뒤 rsp보다 위쪽 미매핑 page를 접근하는 경우 새 페이지를 만듭니다.

    정상적인 stack 접근의 조건은 rsp - 8으로 하고, 새로 매핑하는 메모리 단위 크기 할당은 PGSIZE 단위로 합니다.
    페이지를 4KB 할당/매핑해도 rsp 값은 자동으로 4KB만큼 변하지 않는데,
    커널이 page table에 새 페이지를 매핑하는 일과 CPU의 rsp 레지스터 값을 바꾸는 일은 별개이기 때문입니다.

    ** 큰 흐름 **
    1) 사용자 프로그램이 push 시도
    2) rsp - 8 주소가 매핑 안 되어 page fault
    3) 커널이 해당 4KB page를 매핑
    4) 커널이 user mode로 복귀
    5) CPU가 원래 push 명령 재시도
    6) 그때 rsp가 8바이트 감소
*/

// [헬퍼 함수] stack growth 대상인지 판정하는 함수
static bool
is_stack_growth_candidate (void *addr, void *rsp) {
	if (addr == NULL || rsp == NULL)
		return false;

	// 주소와 rsp가 사용자 영역인지 확인.
	if (!is_user_vaddr (addr) || !is_user_vaddr (rsp))
		return false;

	uint8_t *fault_addr = addr;
	uint8_t *stack_ptr = rsp;
	uint8_t *stack_top = (uint8_t *) USER_STACK;             // 사용자 스택 상한
	uint8_t *stack_bottom = stack_top - USER_STACK_MAX_SIZE; // 사용자 스택 하한

	// 주소가 사용자 스택 하한 이상, 상한 미만 범위 안에 있어야 함.
	if (fault_addr < stack_bottom || fault_addr >= stack_top)
		return false;

	// rsp가 사용자 스택 하한 이상, 상한 이하 범위 안에 있어야 함.
	if (stack_ptr < stack_bottom || stack_ptr > stack_top)
		return false;

	// 실제 검증 조건
	if (fault_addr < stack_ptr - sizeof (char *))
		return false;
	/* rsp-8 보다 아래쪽은 거부하고 그 이상은 정상적인 근방으로서 허용한다.
	    함수가 지역 변수를 쓰거나 push를 하면 stack pointer 주변이나 조금 아래 주소에 접근하는데,
	    push 명령이 보통 8바이트 값을 스택에 저장하기 때문에 return address 크기가 8바이트가 되고,
	    정상적인 push도 기존 rsp보다 8바이트 낮은 주소에서 page fault를 낼 수 있기 때문이다. */

	return true;
}

/* hash.h의 hash_hash_func typedef 이름과 충돌하지 않도록 콜백 함수 이름을 hash_func로 둔다. */
/* SPT에서 page->va를 key로 쓰기 위한 hash 함수.
   hash_elem으로부터 struct page를 꺼내고, va 값으로 hash를 계산한다. */
static uint64_t
hash_func (const struct hash_elem *e,
           void *aux UNUSED) {
	struct page *pp = hash_entry (e, struct page, hash_elem);
	return hash_bytes (&pp->va, sizeof pp->va);
}

/*  page 주소 비교 함수 완성  */
/* hash.h의 hash_less_func typedef 이름과 충돌하지 않도록 콜백 함수 이름을 hash_less로 둔다. */
/* SPT hash table에서 두 page의 va를 비교하는 함수.
   hash table이 page를 넣거나 찾을 때, 두 page 중 어느 va가 더 작은지 판단하는 데 사용한다. */
static bool
hash_less (const struct hash_elem *a,
           const struct hash_elem *b,
           void *aux UNUSED) {
	struct page *pga = hash_entry (a, struct page, hash_elem);
	struct page *pgb = hash_entry (b, struct page, hash_elem);

	return pga->va < pgb->va;
}

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS /* For project 4 */
	pagecache_init ();
#endif
	// register_inspect_intr () 실행시 interrupt를 꺼 둔 상태로 진입함
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* Your code goes here. */
	vm_frame_table_init ();
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
	case VM_UNINIT:
		return VM_TYPE (page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);
static void vm_register_frame (struct frame *frame);
static void vm_unregister_frame (struct frame *frame);
static void vm_free_frame (struct frame *frame);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
                                vm_initializer *init, void *aux) {
	ASSERT (VM_TYPE (type) != VM_UNINIT);
	ASSERT (upage == pg_round_down (upage)); // 정렬 여부 검사

	/* VM_UNINIT */
	bool (*page_initializer) (struct page *, enum vm_type, void *) = NULL;
	switch (VM_TYPE (type)) {
	case VM_ANON:
		page_initializer = anon_initializer;
		break;
	case VM_FILE:
		page_initializer = file_backed_initializer;
		break;
	default:
		goto err;
	}

	/* Check whether the upage is already occupied or not. */
	struct supplemental_page_table *spt = &thread_current ()->spt;
	if (spt_find_page (spt, upage) == NULL) {
		/* Create the page,
		fetch the initialier according to the VM type,*/
		struct page *pp = malloc (sizeof *pp);
		if (pp == NULL) {
			goto err;
		}
		/* and then create "uninit" page struct by calling uninit_new.
		  You should modify the field after calling the uninit_new. */
		uninit_new (pp, upage, init, type, aux, page_initializer);
		pp->writable = writable;

		/* Insert the page into the spt. */
		if (!spt_insert_page (spt, pp)) {
			vm_dealloc_page (pp);
			goto err;
		}
		return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page temp_page = {
		.va = pg_round_down (va),
	};

	struct hash_elem *h_e = hash_find (&spt->spt_hash, &temp_page.hash_elem);

	if (h_e == NULL)
		return NULL;

	// 찾은 hash_elem을 가공해서 찾으려던 page를 반환
	struct page *page = hash_entry (h_e, struct page, hash_elem);

	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt,
                 struct page *page) {
	ASSERT (spt != NULL);
	ASSERT (page != NULL);
	ASSERT (page->va == pg_round_down (page->va)); // 정렬 여부 검사

	struct hash_elem *h_e = &page->hash_elem;

	return hash_insert (&spt->spt_hash, h_e) == NULL;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	ASSERT (spt != NULL);
	ASSERT (page != NULL);

	struct hash *h = &spt->spt_hash;
	struct hash_elem *h_e = &page->hash_elem;
	struct hash_elem *deleted = hash_delete (h, h_e);

	ASSERT (deleted != NULL);

	vm_dealloc_page (page);
	return;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	uint64_t *pml4;
	void *va;
	// 선정한 정책: clock/second-chance

	lock_acquire (&frame_lock);

	while (!list_empty (&frame_table)) {
		struct frame *frame = list_entry (list_pop_front (&frame_table), struct frame, elem);
		frame->in_frame_table = false;

		// frame table에는 유효한 page/owner/pml4를 가진 frame만 있어야 한다.
		ASSERT (frame->page != NULL);
		ASSERT (frame->owner != NULL);
		ASSERT (frame->owner->pml4 != NULL);
		pml4 = frame->owner->pml4;
		va = frame->page->va;

		// second chance 주고 다음 frame으로
		if (pml4_is_accessed (pml4, va)) {
			pml4_set_accessed (pml4, va, false);
			list_push_back (&frame_table, &frame->elem);
			frame->in_frame_table = true;
			continue;
		}
		victim = frame;
		break;
	}
	lock_release (&frame_lock);
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
// 메모리에서 하나의 페이지를 제거한다. 그에 따른 프레임을 반환한다.
// 목적: 메모리 frame만 비우고 page metadata는 SPT에 남겨 두는 것
static struct frame *
vm_evict_frame (void) {
	/* swap out the victim and return the evicted frame. */
	bool succ = false;
	struct frame *victim_frame = vm_get_victim ();
	if (victim_frame == NULL)
		return NULL;

	struct page *victim_page = victim_frame->page;
	if (victim_page == NULL)
		return NULL;

	if (victim_frame->owner == NULL || victim_frame->owner->pml4 == NULL)
		return NULL;

	uint64_t *pml4 = victim_frame->owner->pml4;

	// swap_out(victim->page) 호출. 실패시 NULL 반환
	succ = swap_out (victim_page); // victim page를 swap_out()
	if (!succ) {
		vm_register_frame (victim_frame);
		return NULL;
	}

	// page table mapping 제거
	pml4_clear_page (pml4, victim_page->va);

	// page/frame 연결 해제
	victim_frame->page = NULL;
	victim_page->frame = NULL;
	victim_frame->owner = NULL;

	// evicted frame 반환
	return victim_frame;
}

static void
vm_frame_table_init (void) {
	list_init (&frame_table);
	lock_init (&frame_lock);
}

static void
vm_register_frame (struct frame *frame) {
	if (frame == NULL)
		return;

	lock_acquire (&frame_lock);
	if (!frame->in_frame_table) {
		list_push_back (&frame_table, &frame->elem);
		frame->in_frame_table = true;
	}
	lock_release (&frame_lock);
}

static void
vm_unregister_frame (struct frame *frame) {
	if (frame == NULL)
		return;

	lock_acquire (&frame_lock);
	if (frame->in_frame_table) {
		list_remove (&frame->elem);
		frame->in_frame_table = false;
	}
	lock_release (&frame_lock);
}

static void
vm_free_frame (struct frame *frame) {
	if (frame == NULL)
		return;

	ASSERT (!frame->in_frame_table);

	if (frame->page != NULL && frame->page->frame == frame)
		frame->page->frame = NULL;

	if (frame->kva != NULL)
		palloc_free_page (frame->kva);

	free (frame);
}

/* 새 frame을 할당하거나, user pool이 가득 찬 경우 기존 frame을 eviction해 재사용한다.
 * frame 확보에 실패하면 NULL을 반환한다. */
static struct frame *
vm_get_frame (void) {
	// (참고) 빈 등록 frame은 남기지 않는 정책 채택
	struct frame *frame = malloc (sizeof *frame);
	if (frame == NULL)
		return NULL;

	frame->page = NULL;
	frame->owner = NULL;
	frame->in_frame_table = false;
	frame->kva = palloc_get_page (PAL_USER);
	// eviction 재사용 경로
	if (frame->kva == NULL) {
		free (frame);

		// 기존에 메모리에 적재되어 사용중이던 프레임을 재사용
		frame = vm_evict_frame (); // 기존 frame_table에서 꺼낸 frame을 재사용
		if (frame == NULL)
			return NULL;
	}

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	ASSERT (frame->kva != NULL);

	return frame;
}

/* Growing the stack. */
/* 사용자 스택을 페이지 크기만큼 확장하는 함수.
 * addr가 속한 stack page를 VM_ANON으로 등록하고 즉시 claim한다 */
static bool
vm_stack_growth (void *addr) {
	void *upage = pg_round_down (addr);

	// 할당 실패 시 false를 반환
	if (!vm_alloc_page (VM_ANON | VM_MARKER_0, upage, true))
		return false;

	// claim 실패 시 false를 반환
	if (!vm_claim_page (upage)) {
		struct page *page = spt_find_page (&thread_current ()->spt, upage);
		if (page != NULL)
			spt_remove_page (&thread_current ()->spt, page);
		return false;
	}

	return true;
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
	/* COW 미구현 상태이고 write-protected fault는 미복구 */
	return false;
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f, void *addr, bool user, bool write, bool not_present) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *page = NULL;

	/* Validate the fault */
	// 입력값 검증 로직

	/* 주소 검증 */
	if (addr == NULL || !is_user_vaddr (addr))
		return false;

	/* 권한 위반 fault는 현재 단계에서 복구하지 않는다. */
	if (!not_present) // not_present 가 false이면 '페이지는 있는데 권한 위반'
		return false;

	/* SPT에 등록된 page인지 확인 (swap-out된 anon page, file-backed page도 포함) */
	page = spt_find_page (spt, addr);
	if (page == NULL) {
		/* stack growth 후보인지 검사하는 로직. stack growth 후보이면 확장 시도 기회를 주어야 함. */
		void *rsp = user ? (void *) f->rsp : (void *) thread_current ()->saved_user_rsp;

		if (is_stack_growth_candidate (addr, rsp))
			return vm_stack_growth (addr);

		return false;
	}

	/* read-only page에 write한 경우는 복구하면 안 된다. */
	if (write && !page->writable)
		return false;

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* [헬퍼 함수] frame을 정리하는 함수 */
void
vm_cleanup_page_frame (struct page *page) {
	if (page->frame == NULL)
		return;

	struct frame *frame = page->frame;
	ASSERT (frame->owner != NULL);
	ASSERT (frame->owner->pml4 != NULL);
	ASSERT (frame->in_frame_table);

	pml4_clear_page (frame->owner->pml4, page->va);
	vm_unregister_frame (frame);
	vm_free_frame (frame);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va) {
	struct page *page = NULL;

	page = spt_find_page (&thread_current ()->spt, va);

	if (page == NULL) {
		return false;
	}

	return vm_do_claim_page (page);
}

// [헬퍼 함수] vm_do_claim_page 의 실패 경로에서 롤백하는 함수
static void
vm_rollback_claim_frame (struct frame *frame, struct page *page,
                         bool already_registered) {
	if (frame == NULL)
		return;

	frame->page = NULL;
	frame->owner = NULL;

	if (page != NULL && page->frame == frame)
		page->frame = NULL;

	if (already_registered) {
		vm_unregister_frame (frame);
	}

	vm_free_frame (frame);
}

/* Claim the PAGE and set up the mmu. */
// page와 frame을 연결하는 함수
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	if (frame == NULL)
		return false;

	// 실패 rollback을 위한 등록 상태 기록 (현재 정책상 반환된 frame은 false)
	bool already_registered = frame->in_frame_table;

	/* Set links */
	frame->page = page;
	frame->owner = thread_current ();
	page->frame = frame;

	/* Insert page table entry to map page's VA to frame's PA. */
	bool succ = pml4_set_page (thread_current ()->pml4,
	                           page->va, frame->kva, page->writable);
	if (!succ) {
		vm_rollback_claim_frame (frame, page, already_registered);
		return false;
	}

	// swap in
	succ = swap_in (page, frame->kva);
	if (!succ) {
		pml4_clear_page (thread_current ()->pml4, page->va);
		vm_rollback_claim_frame (frame, page, already_registered);
		return false;
	}

	// claim에 성공한 frame이 frame_table 밖에 있으면 등록한다.
	if (!already_registered) {
		vm_register_frame (frame);
	}

	return true;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	hash_init (&spt->spt_hash,
	           &hash_func, &hash_less, NULL);
}

/* fork를 위해 src SPT를 dst로 복제한다.
 * mmap/file-backed page는 자식에게 상속하지 않는다. */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst,
                              struct supplemental_page_table *src) {
	struct hash_iterator iter;
	hash_first (&iter, &src->spt_hash);

	while (hash_next (&iter)) {
		struct page *page = hash_entry (hash_cur (&iter), struct page, hash_elem);

		switch (VM_TYPE (page->operations->type)) {
		case VM_UNINIT: {
			/* mmap page는 fork 시 자식에게 상속하지 않는다. */
			if (VM_TYPE (page->uninit.type) == VM_FILE) {
				break;
			}

			void *aux = NULL;
			if (page->uninit.aux != NULL) {
				if (VM_TYPE (page->uninit.type) == VM_ANON) {
					aux = lazy_load_aux_duplicate (page->uninit.aux);
				}
				if (aux == NULL)
					return false;
			}

			if (!vm_alloc_page_with_initializer (page->uninit.type, page->va,
			                                     page->writable, page->uninit.init, aux)) {
				lazy_load_aux_destroy (aux);
				return false;
			}

			struct page *child_page = spt_find_page (dst, page->va);
			if (child_page == NULL)
				return false;

			if (!vm_claim_page (page->va)) {
				spt_remove_page (dst, child_page);
				return false;
			}

			break;
		}
		case VM_ANON: {
			if (page->frame == NULL)
				return false;
			if (!vm_alloc_page (VM_ANON, page->va, page->writable))
				return false;
			if (!vm_claim_page (page->va)) {
				struct page *child_page = spt_find_page (dst, page->va);
				if (child_page != NULL)
					spt_remove_page (dst, child_page);
				return false;
			}

			struct page *child_page = spt_find_page (dst, page->va);
			if (child_page == NULL)
				return false;
			if (child_page->frame == NULL) {
				spt_remove_page (dst, child_page);
				return false;
			}

			memcpy (child_page->frame->kva, page->frame->kva, PGSIZE);
			break;
		}
		case VM_FILE:
			break;
		default:
			return false;
		}
	}

	return true;
}

// [헬퍼 함수] SPT hash table 안의 page 하나를 꺼내서 실제로 해제
static void
spt_destroy_page (struct hash_elem *e, void *aux UNUSED) {
	struct page *page = hash_entry (e, struct page, hash_elem); // struct page * 복원
	vm_dealloc_page (page);                                     // vm_dealloc_page 안에서 destroy 실행. destroy 안에서 페이지 종류에 따라 분기 후 물리 프레임 제거
}

/* Free the resource hold by the supplemental page table */
// 현재 프로세스의 SPT 안에 들어 있는 모든 page를 제거/해제한다.
void
supplemental_page_table_kill (struct supplemental_page_table *spt) {
	/* Destroy all the supplemental_page_table hold by thread and
	 * writeback all the modified contents to the storage. */

	// hash_destroy()에 callback을 넘긴다.
	hash_destroy (&spt->spt_hash, spt_destroy_page);
}
