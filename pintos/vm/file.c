/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include <round.h>
#include <string.h>

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
static bool lazy_load_file_page (struct page *page, void *aux);
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
	struct file_page *file_page UNUSED = &page->file;
	return true;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	return true; // 테스트용 임시
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
         struct file *file, off_t offset) {
	uint64_t start = (uint64_t) addr;
	uint64_t end = start + length;
	size_t page_count; // 요청한 mmap 길이를 덮는 page 수
	off_t file_len;    // 원본 파일 길이
	off_t current_offset;
	size_t remaining_file_bytes;
	void *upage;

	// do_mmap 내부 최소 인자 검증
	if (addr == NULL || file == NULL)
		return NULL;

	if (end < start)
		return NULL;

	page_count = DIV_ROUND_UP (length, PGSIZE);
	current_offset = offset;

	// 실제로 매핑할 파일 바이트 수를 요청한 length 이하로 제한
	lock_acquire (&filesys_lock);
	file_len = file_length (file);
	lock_release (&filesys_lock);

	if (file_len <= 0 || offset >= file_len)
		return NULL;

	remaining_file_bytes = file_len - offset;
	if (remaining_file_bytes > length)
		remaining_file_bytes = length;

	// 사용자 페이지마다 하나의 lazy VM_FILE 페이지를 등록
	upage = addr;
	for (size_t i = 0; i < page_count; i++) {
		struct mmap_aux *aux;
		struct file *opened_file;

		aux = malloc (sizeof (struct mmap_aux));
		if (aux == NULL)
			goto fail;

		// 각 page는 cleanup 전까지 독립적인 file reference를 가진다
		lock_acquire (&filesys_lock);
		opened_file = file_duplicate (file); // 이 mmap page가 소유할 file reference
		lock_release (&filesys_lock);

		if (opened_file == NULL) {
			free (aux);
			goto fail;
		}

		// 각 page의 메타데이터 계산
		aux->file = opened_file;
		aux->offset = current_offset;
		aux->read_bytes = remaining_file_bytes < PGSIZE ? remaining_file_bytes : PGSIZE;
		aux->zero_bytes = PGSIZE - aux->read_bytes;
		aux->page_count = page_count;
		aux->map_base = addr;

		// VM_FILE lazy page를 SPT에 등록
		if (!vm_alloc_page_with_initializer (VM_FILE, upage, writable,
		                                     lazy_load_file_page, aux)) {
			lock_acquire (&filesys_lock);
			file_close (opened_file);
			lock_release (&filesys_lock);
			free (aux);
			goto fail;
		}

		// 다음 page 계산을 위해 남은 바이트와 offset을 갱신
		remaining_file_bytes -= aux->read_bytes;
		current_offset += aux->read_bytes;
		upage = (uint8_t *) upage + PGSIZE;
	}

	return addr;

fail:
	// 중간 실패 시 이미 SPT에 등록한 page들을 되돌린다.
	for (void *va = addr; va < upage; va = (uint8_t *) va + PGSIZE) {
		struct page *page = spt_find_page (&thread_current ()->spt, va);
		if (page != NULL)
			spt_remove_page (&thread_current ()->spt, page);
	}

	return NULL;
}

/* Do the munmap */
void
do_munmap (void *addr) {
}
