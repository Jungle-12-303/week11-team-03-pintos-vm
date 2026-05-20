#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
struct page;
enum vm_type;

struct anon_page {
	size_t slot_idx;  // 4KB swap 슬롯 인덱스
	bool in_swapdisk; // 현재 swap disk slot을 소유 중인지 여부
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);
bool anon_copy_from_swap (struct page *src, void *dst_kva);

#endif
