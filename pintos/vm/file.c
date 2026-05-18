/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "filesys/filesys.h"
#include "string.h"

#define PGSIZE (1 << 12)

struct file_page_aux {
	struct file *file;
	off_t offset;
	off_t size;
};

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
static bool lazy_load_file (struct page *page, void *file_page_aux);

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
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page = &page->file;

	off_t read_byte = file_read_at (file_page->file, kva, file_page->size, file_page->offset);
	if (read_byte != page->file.size)
		return false;
	memset (kva + read_byte, 0, PGSIZE - read_byte);

	return true;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page = &page->file;
	file_write ();
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

static bool
lazy_load_file (struct page *page, void *file_page_aux) {
	struct file_page_aux *aux = (struct file_page_aux *) file_page_aux;

	page->file.file = aux->file;
	page->file.offset = aux->offset;
	page->file.size = aux->size;

	free (aux);
	return true;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
         struct file *file, off_t offset) {
	// 1. 인자 검증

	// 2. 매핑이 안되는 경우 검증

	void *start_va = addr;
	int alloc_page_size = 0;
	size_t rest_size = length;

	struct file *saved_file = file_reopen (file);
	ASSERT (saved_file != NULL);

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

			succ = vm_alloc_page_with_initializer (VM_FILE, addr, writable,
			                                       lazy_load_file, aux);

			if (succ == false)
				goto error;
			addr += PGSIZE;
			offset += PGSIZE;
			rest_size -= PGSIZE;
			++alloc_page_size;
		} else {
			aux->size = rest_size;

			succ = vm_alloc_page_with_initializer (VM_FILE, addr, writable,
			                                       lazy_load_file, aux);

			if (succ == false)
				goto error;

			addr += PGSIZE;
			offset += PGSIZE;
			rest_size = 0;
		}
	}

	return start_va;

error:
	if (aux != NULL)
		free (aux);

	// 중간 실패 시 이전 vm_alloc했던 페이지들 dealloc
	void *va = start_va;
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
}
