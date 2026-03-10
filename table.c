#include "compiler.h"

// ==========================================

#define TABLE_SIZE 0x100

typedef struct entry_s
{
	const char     *key;
	void           *data;
	struct entry_s *next;
}entry_t;

struct hashtable_s
{
	entry_t     *buckets[TABLE_SIZE];
	size_t       count;
	size_t       scope;
	bool         fixed;
	void        *defCase; // switch
	bool         hasJumpTable;
	hashtable_t *prev;
};

static unsigned Hash(const char *str);
static unsigned IHash(const char *str); // insensitive

// ==========================================

hashtable_t *TableNew(bool fixed)
{
	hashtable_t *table;

	table = IAlloc(sizeof(*table), fixed ? ARENA_FIXED : ARENA_MISC);

	memset(table, 0, sizeof(*table));

	table->fixed = true;

	return table;
}

void TableAdd(hashtable_t *table, const char *key, void *data)
{
	unsigned hash;
	entry_t *entry;

	if (!table || !key)
		return;

	hash = Hash(key);

	entry = IAlloc(sizeof(*entry), table->fixed ? ARENA_FIXED : ARENA_MISC);

	entry->key  = key;
	entry->data = data;
	entry->next = table->buckets[hash];

	table->buckets[hash] = entry;

	table->count++;
}

void *TableGet(hashtable_t *table, const char *key, bool localSearch)
{
	unsigned hash;

	if (!table || !key)
		return NULL;

	hash = Hash(key);

	do {
		for (entry_t *entry = table->buckets[hash]; entry; entry = entry->next)
			if(entry->key == key)
				return entry->data;
		table = table->prev;
	} while (table && !localSearch);

	return NULL;
}

void TablePush(hashtable_t **table)
{
	hashtable_t *tmph;

	if (!table || !*table || !(tmph = TableNew(false)))
		return;

	tmph->prev  = *table;
	tmph->scope = (*table)->scope + 1;
	*table = tmph;
}

void TablePop(hashtable_t **table)
{
	if (!table || !*table || !(*table)->prev)
		return;

	*table = (*table)->prev;
}

size_t TableGetScope(hashtable_t *table)
{
	if (!table)
		return 0;

	return table->scope;
}

void *TableGetDefault(hashtable_t *table)
{
	if (!table)
		return NULL;

	return table->defCase;
}

void TableSetDefault(hashtable_t *table, void *ptr)
{
	if (!table)
		return;

	table->defCase = ptr;
}

void CompilerAddSymbol(compiler_t *cmp, symbol_t *sym)
{
	if (!cmp || !sym)
		return;

	if (CompilerGetSymbol(cmp, sym->name, true) && cmp->flags & CMPF_LOCAL_SCOPE) {
		Error(cmp, 0, "symbol '%s' redefinition\n", sym->name);
		return;
	}

	if (sym->storage == KW_STATIC && !(cmp->flags & CMPF_LOCAL_SCOPE)) {
		TableAdd(cmp->file->statics, sym->name, sym);
		return;
	}

	TableAdd(cmp->table[TABLE_SYMBOLS], sym->name, sym);
}

symbol_t *CompilerGetSymbol(compiler_t *cmp, const char *name, bool localSearch)
{
	symbol_t *sym;

	if (!cmp || !name)
		return NULL;

	if (sym = TableGet(cmp->file->statics, name, true))
		return sym;

	return TableGet(cmp->table[TABLE_SYMBOLS], name, localSearch);
}

void CompilerNewScope(compiler_t *cmp)
{
	if (!cmp)
		return;

	if (cmp->flags & CMPF_DONT_ENTER_SCOPE) {
		cmp->flags &= ~CMPF_DONT_ENTER_SCOPE;
		return;
	}

	for (int i = 0; i < TABLE_MAX - 1; i++)
		TablePush(&cmp->table[i]);
}

void CompilerDelScope(compiler_t *cmp)
{
	if (!cmp)
		return;

	for (int i = 0; i < TABLE_MAX - 1; i++)
		TablePop(&cmp->table[i]);
}

void *TableIGet(hashtable_t *table, const char *key)
{
	unsigned hash;

	if (!table || !key)
		return NULL;

	hash = IHash(key);

	for (entry_t *entry = table->buckets[hash]; entry; entry = entry->next)
		if (IStrCmp(entry->key ,key))
			return entry->data;

	return NULL;
}

void **TableToArray(hashtable_t *table)
{
	void **array;

	if (!table)
		return NULL;

	array = (void **)Alloc(sizeof(void *) * table->count);

	for (size_t i = 0, j = 0; i < 0x100 && j < table->count; i++) {
		for (entry_t *entry = table->buckets[i]; entry; entry = entry->next) {
			if (table->defCase == entry->data)
				continue;
			array[j++] = entry->data;
		}
	}

	return array;
}

size_t TableGetSize(hashtable_t *table)
{
	if (!table)
		return 0;

	return table->count;
}

bool TableGetJumpTable(hashtable_t *table)
{
	if (!table)
		return false;

	return table->hasJumpTable;
}

void TableSetJumpTable(hashtable_t *table)
{
	if (!table)
		return;

	table->hasJumpTable = true;
}

static unsigned Hash(const char *str)
{
	unsigned hash = 5381;

	if (!str)
		return 0;

	while (*str)
		hash = ((hash << 5) + hash) + *str++;

	hash &= 0xFF;

	return hash;
}

static unsigned IHash(const char *str)
{
	unsigned hash = 5381;

	if (!str)
		return 0;

	while (*str)
		hash = ((hash << 5) + hash) + (*str++ & 0xDF);

	return hash & 0xFF;
}