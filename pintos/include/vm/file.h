#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page { // page fault 후 VM_FILE page가 평생 들고 있는 metadata
	struct file *file;
	off_t offset;
	size_t read_bytes;
	size_t zero_bytes;
	void *map_base;    //  mmap 영역의 시작 주소. munmap 시 사용
	size_t page_count; //  mmap 영역의 전체 page 수.  munmap 시 사용
};

struct mmap_aux { //  page fault 전까지 들고 있는 임시 운반 상자
	struct file *file;
	off_t offset;
	size_t read_bytes;
	size_t zero_bytes;
	void *map_base;
	size_t page_count;
};

void
vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap (void *addr, size_t length, int writable,
               struct file *file, off_t offset);
void do_munmap (void *va);
#endif
