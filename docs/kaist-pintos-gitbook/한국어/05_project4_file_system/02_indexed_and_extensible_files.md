# 02_indexed_and_extensible_files

## 인덱스 기반 확장 가능 파일

기본 파일 시스템은 파일을 하나의 연속 extent로 할당하므로 external fragmentation에 취약합니다. 즉, *n*개의 블록이 비어 있어도 *n*-block 파일을 할당하지 못할 수 있습니다. 이 문제를 해결하기 위해 on-disk inode 구조를 수정하세요.

실제로는 direct block, indirect block, doubly indirect block을 사용하는 인덱스 구조를 뜻하는 경우가 많습니다. 이전 학기에는 대부분의 학생이 *Berkeley UNIX FFS*의 multi-level indexing과 비슷한 방식을 채택했습니다. 하지만 이번에는 구현을 더 쉽게 하기 위해 FAT 방식으로 구현하게 합니다. **반드시 제공된 skeleton code를 사용해 FAT를 구현해야 합니다.** 
코드에 multi-level indexing(강의에서 다룬 FFS)을 위한 코드는 **절대 포함되어서는 안 됩니다**.
이를 어기면 파일 확장 파트 점수는 0점 처리됩니다.

참고로, 파일 시스템 파티션의 크기는 8 MB를 넘지 않는다고 가정해도 됩니다. 메타데이터를 제외한 나머지 전체 파티션 크기만큼 큰 파일까지 지원해야 합니다. 각 inode는 하나의 disk sector에 저장되므로, 담을 수 있는 block pointer 수에는 한계가 있습니다.

### FAT (File Allocation Table)로 큰 파일 인덱싱하기

> 경고: 이 문서는 여러분이 강의에서 일반적인 파일 시스템과 FAT의 기본 원리를 이미 이해했다고 가정합니다. 아직 그렇지 않다면 강의 노트로 돌아가 파일 시스템과 FAT가 무엇인지 먼저 이해하세요.

이전 프로젝트들에서 사용한 기본 파일 시스템에서는 파일이 여러 disk sector에 걸친 하나의 연속된 chunk로 저장되었습니다. 여기서는 *연속 청크 (contiguous chunk)* 를 *cluster*라고 부르겠습니다. 이런 관점에서 보면, 기본 파일 시스템에서 cluster의 크기는 그 cluster에 저장된 파일의 크기와 같았습니다.

external fragmentation을 완화하려면 cluster의 크기를 줄일 수 있습니다(가상 메모리의 page size를 떠올려 보세요). 단순화를 위해 skeleton code에서는 cluster 하나에 들어가는 sector 수를 `1`로 고정해 두었습니다. 이렇게 더 작은 cluster를 사용하면, 하나의 cluster만으로는 파일 전체를 저장하기에 부족할 수 있습니다. 이런 경우 파일 하나에 여러 cluster가 필요하므로, inode 안에서 파일의 cluster들을 인덱싱할 자료구조가 필요합니다. 가장 쉬운 방법 중 하나는 linked-list(즉, *chain*)를 사용하는 것입니다. inode는 파일의 첫 번째 block의 sector 번호를 담고, 첫 번째 block은 두 번째 block의 sector 번호를 담는 식입니다. 하지만 이 순진한 방식은 너무 느립니다. 실제로 필요한 것이 마지막 block뿐이더라도 파일의 모든 block을 읽어야 하기 때문입니다. 이를 해결하기 위해 FAT(File Allocation Table)는 block들 사이의 연결 정보를 block 자체가 아니라 고정 크기의 File Allocation Table에 저장합니다. FAT는 실제 데이터가 아니라 연결 값만 담기 때문에 크기가 작아 DRAM에 캐시해 둘 수 있습니다. 그 결과 테이블의 해당 엔트리만 읽으면 됩니다.

여러분은 `filesys/fat.c`에 제공된 skeleton code를 사용해 inode 인덱싱을 구현하게 됩니다. 이 절에서는 `fat.c`에 이미 구현되어 있는 함수들이 무엇을 하는지, 그리고 여러분이 무엇을 구현해야 하는지를 간단히 설명합니다.

우선 `fat.c`의 6개 함수(즉, `fat_init()`, `fat_open()`, `fat_close()`, `fat_create()`,
`fat_boot_create()`)는 부팅 시점에 디스크를 초기화하고 포맷하므로 수정할 필요가 없습니다. 하지만 `fat_fs_init()`은 직접 작성해야 하며, 이 함수들이 무엇을 하는지 대략 이해해 두면 도움이 됩니다.

---

```c
cluster_t fat_fs_init (void);
```

> FAT 파일 시스템을 초기화합니다. `fat_fs`의 `fat_length`와 `data_start` 필드를 초기화해야 합니다.
> `fat_length`에는 파일 시스템 안의 cluster 수를, `data_start`에는 파일 저장을 시작할 수 있는 sector 위치를 저장합니다. `fat_fs->bs`에 저장된 값들 중 일부를 활용하면 좋을 수 있습니다.
> 이 함수에서 추가로 유용한 데이터를 초기화해도 됩니다.

---

```c
cluster_t fat_create_chain (cluster_t clst);
```

> `clst`(cluster indexing number)로 지정된 cluster 뒤에 새로운 cluster를 하나 붙여 chain을 확장합니다.
> `clst`가 0이면 새 chain을 만듭니다. 새로 할당된 cluster의 번호를 반환합니다.

---

```c
void fat_remove_chain (cluster_t clst, cluster_t pclst);
```

> `clst`부터 시작해 chain에서 cluster들을 제거합니다. `pclst`는 chain에서 바로 앞의 cluster여야 합니다.
> 즉, 이 함수가 끝난 뒤에는 `pclst`가 갱신된 chain의 마지막 원소가 되어야 합니다.
> `clst`가 chain의 첫 번째 원소라면 `pclst`는 0이어야 합니다.

---

```c
void fat_put (cluster_t clst, cluster_t val);
```

> cluster 번호 `clst`가 가리키는 FAT 엔트리를 `val`로 갱신합니다. FAT의 각 엔트리는 chain에서 다음
> cluster를 가리키므로(없다면 `EOChain`), 연결 관계를 갱신하는 데 사용할 수 있습니다.

---

```c
cluster_t fat_get (cluster_t clst);
```

> 주어진 cluster `clst`가 어떤 cluster 번호를 가리키는지 반환합니다.

---

```c
disk_sector_t cluster_to_sector (cluster_t clst);
```

> cluster 번호 `clst`를 대응하는 sector 번호로 변환하고, 그 sector 번호를 반환합니다.

---

기본 파일 시스템을 확장하기 위해 `filesys.c`와 `inode.c`에서 이 함수들을 활용하면 좋을 수 있습니다.

### 파일 확장

**확장 가능한 파일을 구현하세요.**  기본 파일 시스템에서는 파일을 만들 때 파일 크기를 정합니다. 하지만 대부분의 현대 파일 시스템에서는 파일을 처음에 크기 0으로 만들고, 이후 파일 끝을 넘어서는 위치에 쓰기가 일어날 때마다 파일을 확장합니다. 여러분의 파일 시스템도 이 동작을 허용해야 합니다.

파일 크기에는 미리 정해진 한계가 없어야 합니다. 단, 파일 시스템 크기에서 메타데이터를 뺀 크기를 넘을 수는 없습니다. 이는 루트 디렉터리 파일에도 동일하게 적용되며, 이제 루트 디렉터리도 초기 한도인 16개 파일을 넘어 확장될 수 있어야 합니다.

사용자 프로그램은 현재 EOF(end-of-file)를 넘어 `seek`할 수 있습니다. `seek` 자체가 파일을 확장하지는 않습니다. EOF 이후 위치에 쓰기를 하면 파일은 그 위치까지 확장되며, 이전 EOF와 실제 쓰기 시작 위치 사이의 간격은 0으로 채워져야 합니다. EOF를 지난 위치에서 시작하는 read는 어떤 바이트도 반환하지 않습니다.

EOF를 한참 지난 위치에 쓰면, 많은 블록이 전부 0으로만 채워질 수 있습니다. 어떤 파일 시스템은 이렇게 암묵적으로 0이 되는 블록에도 실제 데이터 블록을 할당하고 기록합니다. 다른 파일 시스템은 해당 블록에 명시적으로 쓰기가 일어나기 전까지는 아예 할당하지 않습니다. 후자의 파일 시스템을 "희소 파일 (sparse files)"을 지원한다고 합니다. 여러분의 파일 시스템에서는 어느 쪽 할당 전략을 택해도 됩니다.
