#include "compiler.h"

// ==========================================

static compiler_t *CompilerNew(const char *path);
static void		   CompileFile(const char *path);
static void		   ParseFile(compiler_t *cmp);

// ==========================================

void Boot(int argc, char **argv)
{
	if (!argv || !argc)
		return;

	InitKeywords();
	InitPrecedence();
	InitPrimitives();

	for (int i = 1; i < argc; i++) {
		CompileFile(argv[i]);
		ResetArena(ARENA_MISC);
		ResetArena(ARENA_AST);
		ResetArena(ARENA_IR);
	}

	CompilerShutdown(NULL);
}

static void CompileFile(const char *path)
{
	compiler_t *cmp;

	if (!path || !(cmp = CompilerNew(path)))
		return;

	ParseFile(cmp);

	if (cmp->flags & CMPF_ERROR)
		return;

	GenIR(cmp);

	PrintInstructions(cmp);
}

static void ParseFile(compiler_t *cmp)
{
	node_t **ptr;

	if (!cmp)
		return;

	ptr = &cmp->nodes;

	while (Lex(cmp)) {
		*ptr = ParseTranslationUnit(cmp);
		if (*ptr)
			ptr = &(*ptr)->next_stmt;
		cmp->flags &= ~(CMPF_LOCAL_SCOPE | CMPF_DONT_LEX);
	}
}

static compiler_t *CompilerNew(const char *path)
{
	file_t *file;
	compiler_t *cmp;

	if (!path || !path || !(file = FileNew(path)))
		return NULL;

	cmp = Alloc(sizeof(*cmp));

	memset(cmp, 0, sizeof(*cmp));

	cmp->file = file;

	for (int i = 0; i < TABLE_MAX - 1; i++)
		cmp->table[i] = TableNew(false);

	return cmp;
}