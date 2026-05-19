/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include <round.h>
#include <string.h>
#include <bitmap.h>
#include "devices/disk.h"
#include "threads/mmu.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
static bool lazy_load_file_page (struct page *page, void *aux);
static struct mmap_aux *mmap_aux_create (struct file *file, off_t offset, size_t read_bytes,
                                         size_t zero_bytes, void *map_base, size_t page_count);
static bool mmap_register_page (void *upage, int writable, struct mmap_aux *aux);
static void mmap_rollback_pages (void *start, void *end);

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
// file_backed_initializer는 page의 operations를 file-backed용으로 설정하고,
// page->file metadata를 초기화한다.
bool
file_backed_initializer (struct page *page, enum vm_type type UNUSED, void *kva UNUSED) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
	file_page->file = NULL;
	file_page->map_base = NULL;
	file_page->offset = 0;
	file_page->page_count = 0;
	file_page->read_bytes = 0;
	file_page->zero_bytes = 0;

	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page = &page->file;
	off_t read_amount;

	ASSERT (page != NULL);
	ASSERT (kva != NULL);
	ASSERT (file_page->file != NULL);

	// 파일 내용을 kva에 읽는 작업
	lock_acquire (&filesys_lock);
	read_amount = file_read_at (file_page->file, kva, file_page->read_bytes, file_page->offset);
	lock_release (&filesys_lock);
	if (read_amount != (off_t) file_page->read_bytes) {
		return false;
	}

	// 남은 공간 제로필
	memset ((uint8_t *) kva + file_page->read_bytes, 0, file_page->zero_bytes);

	return true;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	ASSERT (page != NULL);
	ASSERT (page->frame != NULL);
	ASSERT (page->frame->kva);
	ASSERT (page->frame->owner != NULL);
	ASSERT (page->frame->owner->pml4 != NULL);

	struct file_page *file_page = &page->file;
	uint64_t *pml4 = page->frame->owner->pml4;

	ASSERT (file_page != NULL);
	ASSERT (file_page->file != NULL);

	// dirty 페이지를 디스크에 쓴다.
	if (pml4_is_dirty (pml4, page->va)) {
		lock_acquire (&filesys_lock);
		off_t written = file_write_at (file_page->file, page->frame->kva,
		                               file_page->read_bytes, file_page->offset);
		lock_release (&filesys_lock);

		if (written != (off_t) file_page->read_bytes)
			return false;

		// page가 backing file/swap에 반영됐으니, page table entry의 dirty bit를 끈다.
		pml4_set_dirty (pml4, page->va, false);
	}

	return true;
}

/* Destroy the file backed page. PAGE will be freed by the caller. */

static void
file_backed_destroy (struct page *page) {
	// 현재 do_munmap 최소 cleanup
	struct file_page *file_page = &page->file;
	// todo: dirty write-back 함수 작성
	if (page->frame != NULL)
		file_backed_swap_out (page);
	vm_cleanup_page_frame (page); // 페이지와 연결된 프레임만 없애기
	// 파일 닫기
	if (file_page->file != NULL) {
		lock_acquire (&filesys_lock);
		file_close (page->file.file);
		lock_release (&filesys_lock);
		file_page->file = NULL;
	}
}

// [헬퍼 함수] VM_FILE page가 최초 fault-in될 때 파일 내용을 읽고 page->file metadata를 완성하는 file-backed 전용 helper 함수
static bool
lazy_load_file_page (struct page *page, void *aux) {
	struct mmap_aux *m_aux = (struct mmap_aux *) aux;
	void *kva = page->frame->kva;
	off_t read_amount;

	// aux의 메타데이터를 file-backed 페이지로 복사
	page->file.file = m_aux->file;
	page->file.offset = m_aux->offset;
	page->file.read_bytes = m_aux->read_bytes;
	page->file.zero_bytes = m_aux->zero_bytes;
	page->file.map_base = m_aux->map_base;
	page->file.page_count = m_aux->page_count;

	// 파일 내용을 kva에 읽는 작업
	lock_acquire (&filesys_lock);
	read_amount = file_read_at (page->file.file, kva, page->file.read_bytes, page->file.offset);
	lock_release (&filesys_lock);
	if (read_amount != (off_t) page->file.read_bytes) {
		free (m_aux);
		return false;
	}

	// 남은 공간 제로필
	memset ((uint8_t *) kva + page->file.read_bytes, 0, page->file.zero_bytes);

	free (m_aux);
	return true;
}

// [헬퍼 함수] mmap_aux 생성 함수
static struct mmap_aux *
mmap_aux_create (struct file *file, off_t offset, size_t read_bytes,
                 size_t zero_bytes, void *map_base, size_t page_count) {
	struct mmap_aux *aux = malloc (sizeof *aux);
	if (aux == NULL)
		return NULL;

	// 각 page는 cleanup 전까지 독립적인 file reference를 가진다
	lock_acquire (&filesys_lock);
	aux->file = file_duplicate (file);
	lock_release (&filesys_lock);

	if (aux->file == NULL) {
		free (aux);
		return NULL;
	}

	aux->offset = offset;
	aux->read_bytes = read_bytes;
	aux->zero_bytes = zero_bytes;
	aux->map_base = map_base;
	aux->page_count = page_count;

	return aux;
}

// [헬퍼 함수] mmap에 페이지를 등록 여부를 반환하는 함수
static bool
mmap_register_page (void *upage, int writable, struct mmap_aux *aux) {
	if (vm_alloc_page_with_initializer (VM_FILE, upage, writable,
	                                    lazy_load_file_page, aux))
		return true;

	mmap_aux_destroy (aux);
	return false;
}

// [헬퍼 함수] 실패시 rollback 함수
static void
mmap_rollback_pages (void *first, void *limit) {
	struct supplemental_page_table *spt = &thread_current ()->spt;

	// 중간 실패 시 이미 SPT에 등록한 page들을 되돌린다.
	for (void *va = first; va < limit; va = (uint8_t *) va + PGSIZE) {
		struct page *page = spt_find_page (spt, va);
		if (page != NULL)
			spt_remove_page (spt, page);
	}
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
		size_t page_read_bytes;
		if (remaining_file_bytes < PGSIZE) {
			page_read_bytes = remaining_file_bytes;
		} else
			page_read_bytes = PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;
		struct mmap_aux *aux;

		// aux 생성
		aux = mmap_aux_create (file, current_offset, page_read_bytes,
		                       page_zero_bytes, addr, page_count);
		if (aux == NULL)
			goto fail;

		// VM_FILE lazy page를 SPT에 등록
		if (!mmap_register_page (upage, writable, aux))
			goto fail;

		// 다음 page 계산을 위해 남은 바이트와 offset을 갱신
		remaining_file_bytes -= page_read_bytes;
		current_offset += page_read_bytes;
		upage = (uint8_t *) upage + PGSIZE;
	}

	return addr;

fail:
	// 등록된 페이지만 롤백한다.
	// 실패 시 이미 등록된 page 범위: addr <= va < upage < end(전체 요청 끝)
	mmap_rollback_pages (addr, upage);
	return NULL;
}

// [헬퍼 함수] do_munmap() 의 페이지 count를 얻기 위한 함수
static size_t
get_mmap_page_count (struct page *page, void *addr) {
	// munmap 대상 주소가 현재 프로세스의 mmap file-backed page인지 확인
	if (page == NULL || page_get_type (page) != VM_FILE)
		return 0;

	// 아직 page fault가 나지 않은 mmap page는 metadata가 uninit.aux에 남아 있음
	if (page->operations->type == VM_UNINIT) {
		struct mmap_aux *aux = page->uninit.aux;

		if (aux == NULL || aux->map_base != addr)
			return 0;

		return aux->page_count;
	}
	// 이미 fault-in된 mmap page는 metadata가 page->file에 옮겨져 있음
	if (page->file.map_base != addr)
		return 0;

	return page->file.page_count;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *first = spt_find_page (spt, addr);
	size_t page_count = get_mmap_page_count (first, addr);

	if (page_count == 0)
		return;

	// mmap 범위의 page를 순회하며 SPT에서 제거한다.
	for (size_t i = 0; i < page_count; i++) {
		// mmap 범위 안의 다음 page를 찾는다.
		void *va = (uint8_t *) addr + i * PGSIZE;
		struct page *page = spt_find_page (spt, va);

		// page가 file-backed page 인지 확인
		if (page == NULL || page_get_type (page) != VM_FILE)
			continue;

		// 아직 fault-in되지 않은 mmap page는 aux가 file reference를 소유하므로 직접 정리
		if (page->operations->type == VM_UNINIT) {
			struct mmap_aux *aux = page->uninit.aux;

			// page가 같은 mmap 영역에 속한 page인지 확인
			if (aux == NULL || aux->map_base != addr)
				continue;

			mmap_aux_destroy (aux);
			page->uninit.aux = NULL;
		} else {
			/* 이미 fault-in된 mmap page는 metadata가 page->file에 있으므로
			page->file.map_base로 같은 mmap 영역인지 확인한다. */
			if (page->file.map_base != addr)
				continue;
		}
		// SPT 제거가 page destroy까지 수행한다.
		spt_remove_page (spt, page);
	}
}

// [헬퍼 함수] mmap aux 해제 helper 함수
void
mmap_aux_destroy (void *aux_) {
	struct mmap_aux *aux = aux_;

	if (aux == NULL)
		return;

	if (aux->file != NULL) {
		lock_acquire (&filesys_lock);
		file_close (aux->file);
		lock_release (&filesys_lock);
		aux->file = NULL;
	}

	free (aux);
}
