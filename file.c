#include "compiler.h"

// ==========================================

static bool CheckExtension(const char *path);

// ==========================================

file_t *FileNew(const char *path)
{
	file_t *file;
	FILE   *fp;
	size_t  size;

	if (!path || !CheckExtension(path))
		return NULL;

	fp = fopen(path, "rb");

	if (!fp) {
		fprintf(stderr, "Error opening file '%s'\n", path);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);

	size = ftell(fp);

	rewind(fp);

	file = IAlloc(sizeof(*file) + size + 1, ARENA_SOURCE);

	memset(file, 0, sizeof(*file));

	file->path = path;
	file->src = (char *)(file + 1);
	file->line = 1;
	file->statics = TableNew(false);

	fread(file->src, sizeof(*file->src), size, fp);

	file->src[size] = '\0';

	fclose(fp);
	
	return file;
}

static bool CheckExtension(const char *path)
{
	const char *ptr;

	if (!path)
		return false;

	for (ptr = NULL; *path; path++)
		if (*path == '.')
			ptr = path;

	if (!ptr) {
		fprintf(stderr, "Missing extension\n");
		return false;
	}

	if (!IStrCmp(ptr + 1, "c")) {
		fprintf(stderr, "Invalid extension, use .c\n");
		return false;
	}

	return true;
}