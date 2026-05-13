/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include <hash.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "threads/palloc.h"

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
	/* TODO: Your code goes here. */
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

	if (hash_delete (h, h_e) == NULL) {
		printf ("!! [spt_remove_page] 삭제할 page를 찾을 수 없다 \n");
		return;
	}

	vm_dealloc_page (page);
	return;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = malloc (sizeof *frame);
	if (frame == NULL)
		return NULL;

	frame->page = NULL;
	frame->kva = palloc_get_page (PAL_USER);

	if (frame->kva == NULL) {
		free (frame);

		frame = vm_evict_frame ();
		if (frame == NULL)
			return NULL;
	}

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	ASSERT (frame->kva != NULL);

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr, bool user UNUSED, bool write, bool not_present) {
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

	/* SPT에 등록된 lazy page인지 확인 */
	page = spt_find_page (spt, addr);
	if (page == NULL) {
		/* todo: (차후) page == NULL일 때 stack growth 후보인지 검사하는 로직 추가.
		stack growth 후보이면 확장 시도 기회를 주어야 함. */
		return false;
	}

	/* read-only page에 write한 경우는 복구하면 안 된다. */
	if (write && !page->writable)
		return false;

	// todo: 차후 f 와 user 사용시 검증 로직에 추가

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
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

/* Claim the PAGE and set up the mmu. */
// page와 frame을 연결하는 함수
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	if (frame == NULL)
		return false;

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* Insert page table entry to map page's VA to frame's PA. */
	bool succ = pml4_set_page (thread_current ()->pml4,
	                           page->va, frame->kva, page->writable);
	if (!succ) {
		frame->page = NULL;
		page->frame = NULL;
		palloc_free_page (frame->kva);
		free (frame);
		return false;
	}

	succ = swap_in (page, frame->kva);
	if (!succ) {
		pml4_clear_page (thread_current ()->pml4, page->va);
		frame->page = NULL;
		page->frame = NULL;
		palloc_free_page (frame->kva);
		free (frame);
		return false;
	}

	return true;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	hash_init (&spt->spt_hash,
	           &hash_func, &hash_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
                              struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
