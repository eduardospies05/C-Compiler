#include "compiler.h"

// ==========================================

typedef struct string_s
{
	char  *str;
	size_t len;
	struct string_s *next;
}string_t;

static string_t *buckets[1024];

// ==========================================

char *Atom(const char *str)
{
	if (!str)
		CompilerShutdown("Atom operation error\n");

	return AtomLength(str, strlen(str));
}

char *AtomI32(long num)
{
	unsigned value;
	char buffer[45];
	char *ptr = buffer + sizeof(buffer);

	value = (num < 0) ? -num : num;

	do
		*--ptr = value % 10 + '0';
	while((value /= 10) > 0);

	if (num < 0)
		*--ptr = '-';

	return AtomLength(ptr, buffer + sizeof(buffer) - ptr);
}

char *AtomLength(const char *str, size_t length)
{
	string_t   *entry;
	unsigned    hash = 5381;
	const char *endAddr = str;

	if (!str || !length)
		CompilerShutdown("Atom operation error\n");

	for (size_t i = 0; i < length; i++)
		hash = ((hash << 5) + hash) + *endAddr++;

	hash &= 0x3FF;

	for (entry = buckets[hash]; entry; entry = entry->next) {
		const char *s1, *s2;

		if (entry->len != length)
			continue;

		s1 = str;
		s2 = entry->str;

		do
			if(s1 == endAddr)
				return entry->str;
		while(*s1++ == *s2++);
	}

	entry = Alloc(sizeof(*entry) + length + 1);

	entry->str = (char *)(entry + 1);
	entry->len = length;
	entry->next = buckets[hash];

	buckets[hash] = entry;

	memcpy(entry->str, str, length);

	entry->str[length] = '\0';

	return entry->str;
}