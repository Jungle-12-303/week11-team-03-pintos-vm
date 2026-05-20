/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "filesys/filesys.h"
#include "string.h"
#include "threads/vaddr.h"

#define PGSIZE                  (1 << 12)
#define IS_ADDR_ALIGN(addr)     (addr == pg_round_down (addr))
#define IS_OFFSET_ALIGN(offset) (offset == pg_round_down (offset))

struct file_page_aux {
	struct file *file;
	off_t offset;
	off_t size;
};

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
static bool lazy_load_file (struct page *page, void *file_page_aux);
static bool set_page_mapping_info (void *va, void *mapping_start, size_t mapping_length);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
	// printf ("!![vm_file_init] vm 초기화\n");
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;
	struct file_page *file_page = &page->file;

	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page = &page->file;

	off_t read_byte = file_read_at (file_page->file, kva, file_page->size, file_page->offset);
	if (read_byte != file_page->size)
		return false;

	memset ((uint8_t *) kva + read_byte, 0, PGSIZE - read_byte);

	return true;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page = &page->file;

	if (page->frame == NULL)
		return true;

	struct thread *owner_t = page->frame->owner_thread;

	if (pml4_is_dirty (owner_t->pml4, page->va) == true) {
		// printf ("!![file_backed_swap_out] 파일 내용 변경\n");
		off_t write_byte = file_write_at (file_page->file, page->frame->kva, file_page->size, file_page->offset);
		pml4_set_dirty (owner_t->pml4, page->va, false);
	}
	return true;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	printf ("!![file_backed_destroy]\n");
	file_backed_swap_out (page);
}

static bool
lazy_load_file (struct page *page, void *file_page_aux) {
	struct file_page_aux *aux = (struct file_page_aux *) file_page_aux;

	page->file.file = aux->file;
	page->file.offset = aux->offset;
	page->file.size = aux->size;
	file_backed_swap_in (page, page->frame->kva);

	free (aux);
	return true;
}

static bool
set_page_mapping_info (void *va, void *mapping_start, size_t mapping_length) {
	struct page *page = spt_find_page (&thread_current ()->spt, va);

	if (page == NULL)
		return false;

	page->mapping_start = mapping_start;
	page->mapping_length = mapping_length;

	return true;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
         struct file *file, off_t offset) {
	// 1. 인자 검증
	if (is_kernel_vaddr (addr) || !IS_ADDR_ALIGN (addr) ||
	    length == 0 || file == NULL || !IS_OFFSET_ALIGN (offset))
		return NULL;

	// 2. 메모리 매핑하기
	void *cur_addr = addr;
	int alloc_page_size = 0;
	size_t rest_size = length;

	struct file *saved_file = file_reopen (file);
	// ASSERT (saved_file != NULL);

	struct file_page_aux *aux;
	bool succ = false;

	while (rest_size > 0) {
		// 페이지 만들기
		aux = malloc (sizeof *aux);
		if (aux == NULL)
			goto error;

		aux->file = saved_file;
		aux->offset = offset;

		if (rest_size >= PGSIZE) {
			aux->size = PGSIZE;

			succ = vm_alloc_page_with_initializer (VM_FILE, cur_addr, writable,
			                                       lazy_load_file, aux);
			if (succ == false) {
				// printf ("[!!] 풀페이지 매핑 시도 실패 \n");
				goto error;
			}

			cur_addr += PGSIZE;
			offset += PGSIZE;
			rest_size -= PGSIZE;
			++alloc_page_size;
		} else {
			aux->size = rest_size;

			succ = vm_alloc_page_with_initializer (VM_FILE, cur_addr, writable,
			                                       lazy_load_file, aux);

			if (succ == false) {
				// printf ("[!!] 부분 페이지(마지막) 매핑 시도 실패 \n");
				goto error;
			}

			cur_addr += PGSIZE;
			offset += PGSIZE;
			rest_size = 0;
		}

		// 증가하기 전 작업하던 페이지 매핑 멤버에 값 할당
		if (set_page_mapping_info (cur_addr - PGSIZE, addr, length) == false)
			goto error;
	}

	return addr;

error:
	// printf ("[!!] 매핑 실패\n");
	if (aux != NULL)
		free (aux);

	// 중간 실패 시 이전 vm_alloc했던 페이지들 dealloc
	void *va = addr;
	while (alloc_page_size != 0) {
		struct page *page = spt_find_page (&thread_current ()->spt, va);
		spt_remove_page (&thread_current ()->spt, page);
		alloc_page_size -= 1;
		va += PGSIZE;
	}

	return NULL;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	// 1. 인자 검증
	if (is_kernel_vaddr (addr))
		return;

	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *page = spt_find_page (&thread_current ()->spt, addr);
	if (page == NULL)
		return;
	void *cur_addr = page->mapping_start;
	off_t length = page->mapping_length;

	size_t total_page = (length + PGSIZE - 1) / PGSIZE;

	while (total_page > 0) {
		page = spt_find_page (&thread_current ()->spt, cur_addr);
		if (page == NULL)
			return;
		spt_remove_page (&thread_current ()->spt, page);

		cur_addr += PGSIZE;
		--total_page;
	}
}
