/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <bitmap.h>
#include <string.h>

#define SECTORS_PER_PAGE (PGSIZE / DISK_SECTOR_SIZE)

// swap slot 관리를 위한 전역 상태
static struct bitmap *swap_bitmap;
static struct lock swap_lock; // bitmap 수정을 원자적으로 보장하기 위한 락

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* Set up the swap_disk. */
	swap_disk = disk_get (1, 1); // pintos 에서 약정된 방식으로 디스크 공간 지정
	if (swap_disk == NULL)
		PANIC ("swap disk not found");

	size_t slot_count = disk_size (swap_disk) / SECTORS_PER_PAGE; // 총 슬롯 갯수

	/*  swap_bitmap은 "swap slot 사용 현황표"이다.
	    swap disk의 각 4KB slot이 사용 중인지 아닌지 기록한다. */
	lock_init (&swap_lock); // 비트맵 수정을 위한 락 생성
	swap_bitmap = bitmap_create (slot_count);
	if (swap_bitmap == NULL)
		PANIC ("swap bitmap creation failed");
}

// anon_initializer 는 uninit page를 anonymous page로 바꾸는 초기화 함수
bool
anon_initializer (struct page *page, enum vm_type type UNUSED, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops; // 이 page의 동작 함수를 anonymous page용 함수 테이블로 바꾸기

	// anon_page 초기화
	// (참고) BITMAP_ERROR 의 값은 size_t가 표현할 수 있는 최댓값이지만 의미는 '에러 값'으로 사용
	struct anon_page *anon_page = &page->anon;
	anon_page->slot_idx = BITMAP_ERROR;
	anon_page->in_swapdisk = false;
	memset (kva, 0, PGSIZE); // 제로필 초기화

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	ASSERT (page != NULL);
	ASSERT (kva != NULL);
	ASSERT (swap_bitmap != NULL);

	struct anon_page *anon_page = &page->anon;

	ASSERT (anon_page->in_swapdisk);
	ASSERT (anon_page->slot_idx != BITMAP_ERROR);
	ASSERT (anon_page->slot_idx < bitmap_size (swap_bitmap));

	disk_sector_t sector = anon_page->slot_idx * SECTORS_PER_PAGE; // 시작 섹터 인덱스

	// slot_idx로부터 sector 시작 위치를 계산해서 frame으로 복원
	for (size_t i = 0; i < SECTORS_PER_PAGE; i++)
		disk_read (swap_disk, sector + i, (uint8_t *) kva + i * DISK_SECTOR_SIZE);

	// 성공 후 bitmap slot 해제
	lock_acquire (&swap_lock);
	bitmap_reset (swap_bitmap, anon_page->slot_idx);
	lock_release (&swap_lock);
	anon_page->slot_idx = BITMAP_ERROR;
	anon_page->in_swapdisk = false;

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	ASSERT (page != NULL);

	struct anon_page *anon_page = &page->anon;

	ASSERT (page->frame != NULL);
	ASSERT (page->frame->kva);
	ASSERT (swap_bitmap != NULL);
	ASSERT (anon_page->in_swapdisk == false);

	// 슬롯 확보
	lock_acquire (&swap_lock);
	// swap_bitmap에서 0번 index부터 시작해서, false인 bit 1개를 찾고, 찾으면 그 bit를 true로 바꾼 뒤 그 index를 반환
	size_t slot = bitmap_scan_and_flip (swap_bitmap, 0, 1, false);
	lock_release (&swap_lock);

	if (slot == BITMAP_ERROR)
		PANIC ("swap disk full");

	disk_sector_t sector = slot * SECTORS_PER_PAGE; // slot index를 swap disk의 시작 sector 번호로 변환

	// frame의 0~4095바이트를 각각의 섹터별로 512B씩 쪼개서 쓰기 실행
	// (uint8_t *)로 캐스팅하는 이유는 포인터 연산을 바이트 단위로 하기 위함이다.
	for (size_t i = 0; i < SECTORS_PER_PAGE; i++)
		disk_write (swap_disk, sector + i, (uint8_t *) page->frame->kva + i * DISK_SECTOR_SIZE);

	anon_page->slot_idx = slot;
	anon_page->in_swapdisk = true;

	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
/* anon_destroy()의 역할은 특정한 page가 anonymous page로서 들고 있던 것을 반납하는 것.
    그 page가 현재 swap disk slot을 소유 중일 경우 그 slot 하나를 반환 */
// swap disk 전체나 비트맵을 초기화하지 않고, 개별 페이지 단위로 접근하는 이유는 사라지는 page가 실제로 소유한 slot 하나만 반환하기 위해서이다.
static void
anon_destroy (struct page *page) {
	ASSERT (page != NULL);
	ASSERT (swap_bitmap != NULL);

	struct anon_page *anon_page = &page->anon;

	// 더 이상 사용하지 않는 anon_page에서 추출한 데이터가 swap disk에 남아있을 경우 비트맵의 해당 영역이 비어있다고 표시해주어야 함.
	if (anon_page->in_swapdisk) {
		lock_acquire (&swap_lock);
		bitmap_reset (swap_bitmap, anon_page->slot_idx);
		lock_release (&swap_lock);
		anon_page->slot_idx = BITMAP_ERROR;
		anon_page->in_swapdisk = false;
	}

	vm_cleanup_page_frame (page); // 프레임 제거 함수
}
