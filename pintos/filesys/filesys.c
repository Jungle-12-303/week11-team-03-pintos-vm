#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the file system module.
 * If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) {
	filesys_disk = disk_get (0, 1);
	if (filesys_disk == NULL)
		PANIC ("hd0:1 (hdb) not present, file system initialization failed");

	inode_init ();

#ifdef EFILESYS
	fat_init ();

	if (format)
		do_format ();

	fat_open ();
#else
	/* Original FS */
	free_map_init ();

	if (format)
		do_format ();

	free_map_open ();
#endif
}

/* Shuts down the file system module, writing any unwritten data
 * to disk. */
void
filesys_done (void) {
	/* Original FS */
#ifdef EFILESYS
	fat_close ();
#else
	free_map_close ();
#endif
}

/* Creates a file named NAME with the given INITIAL_SIZE.
 * Returns true if successful, false otherwise.
 * Fails if a file named NAME already exists,
 * or if internal memory allocation fails. */
/* 지정된 초기 크기(INITIAL_SIZE)로 NAME이라는 이름의 파일을 생성합니다.
 * 성공하면 true, 실패하면 false를 반환합니다.
 * NAME이라는 이름의 파일이 이미 존재하거나,
 * 내부 메모리 할당에 실패하면 실패합니다. */
bool
filesys_create (const char *name, off_t initial_size) {
	disk_sector_t inode_sector = 0;
	// 유저가 요청하는 디렉토리 찾고 있는지 점검
	// pintos는 루트 디렉토리 말고 사용 안함. 
	struct dir *dir = dir_open_root ();
	bool success = (dir != NULL
			&& free_map_allocate (1, &inode_sector) 		// 빈 섹터 찾고 inode_sector에 반환
			&& inode_create (inode_sector, initial_size) 	// 메타데이터 생성
			&& dir_add (dir, name, inode_sector)			// 디렉토리 생성
			&& *name != NULL);								// 이름이 NULL이 아닐 때
	// 실패하거나 inode 못찾으면 free			
	if (!success && inode_sector != 0)
		free_map_release (inode_sector, 1);
	dir_close (dir);

	return success;
}

/* Opens the file with the given NAME.
 * Returns the new file if successful or a null pointer
 * otherwise.
 * Fails if no file named NAME exists,
 * or if an internal memory allocation fails. */
/* 지정된 NAME이라는 이름의 파일을 엽니다.
 * 성공하면 새 파일을 반환하고, 그렇지 않으면 null 포인터를 반환합니다.
 * NAME이라는 이름의 파일이 존재하지 않거나,
 * 또는 내부 메모리 할당에 실패하면 실패합니다. */
struct file *
filesys_open (const char *name) {
	struct dir *dir = dir_open_root ();
	struct inode *inode = NULL;

	if (dir != NULL)
		dir_lookup (dir, name, &inode);
	dir_close (dir);

	return file_open (inode);
}

/* Deletes the file named NAME.
 * Returns true if successful, false on failure.
 * Fails if no file named NAME exists,
 * or if an internal memory allocation fails. */
/* NAME이라는 이름의 파일을 삭제합니다.
 * 성공하면 true, 실패하면 false를 반환합니다.
 * NAME이라는 이름의 파일이 존재하지 않거나,
 * 내부 메모리 할당에 실패하면 실패합니다. */
bool
filesys_remove (const char *name) {
	struct dir *dir = dir_open_root ();
	bool success = dir != NULL && dir_remove (dir, name);
	dir_close (dir);

	return success;
}

/* Formats the file system. */
/* 파일 시스템을 포맷합니다. */
static void
do_format (void) {
	printf ("Formatting file system...");

#ifdef EFILESYS
	/* Create FAT and save it to the disk. */
	fat_create ();
	fat_close ();
#else
	free_map_create ();
	if (!dir_create (ROOT_DIR_SECTOR, 16))
		PANIC ("root directory creation failed");
	free_map_close ();
#endif

	printf ("done.\n");
}
