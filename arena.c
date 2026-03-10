#include "compiler.h"

// ==========================================

#define MAGIC_SIGNATURE 0xCAFE
#define ARENA_SIZE (8 * 1024) // 8KB

typedef struct block_s
{
	byte  *ptr;
	byte  *limit;
	word   magic;
	struct block_s *next;
}block_t;

static block_t *arena[ARENA_MAX];

static block_t *NewBlock(size_t nbytes);

// ==========================================

static void *Malloc(size_t nbytes);
static void  Free(void *ptr);
static void  CheckIndex(size_t index);
static void  FreeArena(size_t index);

// ==========================================

void *Alloc(size_t nbytes)
{
	return IAlloc(nbytes, ARENA_MISC);
}

void *IAlloc(size_t nbytes, size_t index)
{
	block_t *blk;
	size_t   align;

	CheckIndex(index);

	align = Align(nbytes, sizeof(void *));
	blk   = arena[index];

	if (blk && blk->ptr + align <= blk->limit)
		return blk->ptr += align, blk->ptr - align;

	if(nbytes < ARENA_SIZE)
		nbytes = ARENA_SIZE;

	blk = NewBlock(nbytes);

	blk->next = arena[index];

	arena[index] = blk;

	return blk->ptr += align, blk->ptr - align;
}

static block_t *NewBlock(size_t nbytes)
{
	block_t *blk;

	blk = Malloc(sizeof(*blk) + nbytes);

	blk->ptr   = (byte *)(blk + 1);
	blk->limit = (blk->ptr + nbytes);
	blk->magic = MAGIC_SIGNATURE;
	blk->next  = NULL;

	return blk;
}

void ResetArena(size_t index)
{
	block_t *blk;

	if (!arena[index])
		return;

	CheckIndex(index);

	blk = arena[index];

	arena[index] = arena[index]->next;

	FreeArena(index);

	blk->ptr  = (byte *)(blk + 1);
	blk->next = NULL;

	arena[index] = blk;
}

void DestroyAll(void)
{
	for (size_t i = 0; i < ARENA_MAX; i++)
		FreeArena(i);
}

static void *Malloc(size_t nbytes)
{
	void *ptr;

	ptr = malloc(nbytes);

	if (!ptr)
		CompilerShutdown("Error allocating memory\n");

	return ptr;
}

static void Free(void *ptr)
{
	if (!ptr)
		return;

	free(ptr);
}

static void CheckIndex(size_t index)
{
	if (index < ARENA_MAX)
		return;

	CompilerShutdown("Invalid arena index\n");
}

static void FreeArena(size_t index)
{
	CheckIndex(index);

	for (block_t *tmp; arena[index]; arena[index] = tmp) {
		tmp = arena[index]->next;
		if (arena[index]->magic == MAGIC_SIGNATURE) {
			Free(arena[index]);
			continue;
		}
		fprintf(stderr, "bad memory signature\n");
	}
}