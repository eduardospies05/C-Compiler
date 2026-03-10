#include "compiler.h"

// ==========================================

static node_t *ParseBlock(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseBlockStmt(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseWhileStmt(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseDoWhile(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseIfStmt(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseBreakStmt(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseContinueStmt(compiler_t *cmp);
static node_t *ParseForStmt(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseForInit(compiler_t *cmp);
static node_t *ParseReturnStmt(compiler_t *cmp);
static node_t *ParseIdentifier(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseGotoStmt(compiler_t *cmp);
static node_t *ParseCaseStmt(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseDefaultStmt(compiler_t *cmp, hashtable_t *cases);
static node_t *ParseSwitchStmt(compiler_t *cmp);

// ==========================================

node_t *ParseStmt(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node;

	if (!cmp || cmp->token == ';')
		return NULL;

	switch (cmp->token) {
		case '{':
			return ParseBlock(cmp, cases);
		case KW_WHILE:
			return ParseWhileStmt(cmp, cases);
		case KW_DO:
			return ParseDoWhile(cmp, cases);
		case KW_IF:
			return ParseIfStmt(cmp, cases);
		case KW_BREAK:
			return ParseBreakStmt(cmp, cases);
		case KW_CONTINUE:
			return ParseContinueStmt(cmp);
		case KW_FOR:
			return ParseForStmt(cmp, cases);
		case KW_RETURN:
			return ParseReturnStmt(cmp);
		case TK_ID:
			return ParseIdentifier(cmp, cases);
		case KW_GOTO:
			return ParseGotoStmt(cmp);
		case KW_CASE:
			return ParseCaseStmt(cmp, cases);
		case KW_DEFAULT:
			return ParseDefaultStmt(cmp, cases);
		case KW_SWITCH:
			return ParseSwitchStmt(cmp);
	}

	node = ParseExpr(cmp, 0);

	Expect(cmp, ';');

	return node;
}

static node_t *ParseSwitchStmt(compiler_t *cmp)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(SWITCH, cmp->file->line);

	Lex(cmp);

	Accept(cmp, '(');

	node->_switch.cond = ParseExpr(cmp, 0);
	AnalyseCondition(cmp, node->_switch.cond);

	Accept(cmp, ')');

	node->_switch.cases_table = TableNew(false);

	node->_switch.body = ParseStmt(cmp, node->_switch.cases_table);

	return node;
}

static node_t *ParseDefaultStmt(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node;

	if (!cmp)
		return NULL;
	
	node = NodeNew(DEFAULT, cmp->file->line);

	if (!cases)
		Error(cmp, 0, errorTable[DEFAULT_OUTSIDE_OF_SWITCH]);

	if (TableGetDefault(cases))
		Error(cmp, 0, errorTable[DEFAULT_ALREADY_DECLARED]);
	else
		TableSetDefault(cases, node);

	Lex(cmp);

	Accept(cmp, ':');

	node->_case.parentSwitch = cases;

	node->_case.statement = ParseStmt(cmp, cases);

	return node;
}

static node_t *ParseCaseStmt(compiler_t *cmp, hashtable_t *cases)
{
	node_t     *node, *tmp;
	size_t      num;
	const char *key;

	if (!cmp)
		return NULL;

	if (!cases)
		Error(cmp, 0, errorTable[CASE_OUTSIDE_OF_SWITCH]);

	node = NodeNew(CASE, cmp->file->line);

	Lex(cmp);

	node->_case.cond = ParseExpr(cmp, 0);

	num = SolveExpr(cmp, node->_case.cond);

	node->_case.valCond = num;

	key = AtomI32(num);

	if (tmp = TableGet(cases, key, true))
		Error(cmp, 0, "case '%s' already declared at line %i\n", key, tmp->line);
	else
		TableAdd(cases, key, node);

	Accept(cmp, ':');

	node->_case.parentSwitch = cases;

	node->_case.statement = ParseStmt(cmp, cases);

	return node;
}

static node_t *ParseForStmt(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(FOR, cmp->file->line);

	Lex(cmp);

	Accept(cmp, '(');

	CompilerNewScope(cmp);

	node->_for.init = ParseForInit(cmp);

	Accept(cmp, ';');

	if (cmp->token != ';') {
		node->_for.cond = ParseExpr(cmp, 0);
		AnalyseCondition(cmp, node->_for.cond);
	}

	Accept(cmp, ';');

	if (cmp->token != ')')
		node->_for.step = ParseExpr(cmp, 0);

	Accept(cmp, ')');

	cmp->lpCount++;

	node->_for.then = ParseStmt(cmp, cases);

	cmp->lpCount--;

	CompilerDelScope(cmp);

	return node;
}

static node_t *ParseForInit(compiler_t *cmp)
{
	if (!cmp || cmp->token == ';')
		return NULL;

	if (IsType(cmp) || IsStorage(cmp))
		return ParseDeclaration(cmp);

	return ParseExpr(cmp, 0);
}

static node_t *ParseReturnStmt(compiler_t *cmp)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(RETURN, cmp->file->line);

	if (Lex(cmp) == ';') {
		node->type = cmp_primitives[VOID];
		AnalyseReturn(cmp, node);
		return node;
	}

	node->unary.expr = ParseExpr(cmp, 0);

	if (node->unary.expr)
		node->type = node->unary.expr->type;

	Expect(cmp, ';');

	AnalyseReturn(cmp, node);
	return node;
}

static node_t *ParseIfStmt(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(IF, cmp->file->line);

	Lex(cmp);

	Accept(cmp, '(');

	node->_if.cond = ParseExpr(cmp, 0);

	AnalyseCondition(cmp, node->_if.cond);

	Accept(cmp, ')');

	node->_if.then = ParseStmt(cmp, cases);

	if (Lex(cmp) == KW_ELSE) {
		Lex(cmp);
		node->_if._else = ParseStmt(cmp, cases);
		return node;
	}

	cmp->flags |= CMPF_DONT_LEX;
	return node;
}

static node_t *ParseWhileStmt(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(WHILE, cmp->file->line);

	Lex(cmp);

	Accept(cmp, '(');

	node->_while.cond = ParseExpr(cmp, 0);

	AnalyseCondition(cmp, node->_while.cond);

	Accept(cmp, ')');

	cmp->lpCount++;

	node->_while.then = ParseStmt(cmp, cases);

	cmp->lpCount--;

	return node;
}

static node_t *ParseDoWhile(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(DO_WHILE, cmp->file->line);

	Lex(cmp);

	cmp->lpCount++;

	node->_while.then = ParseStmt(cmp, cases);

	cmp->lpCount--;

	Lex(cmp);

	if (cmp->token != KW_WHILE)
		Error(cmp, 0, "while keyword expected\n");

	Lex(cmp);

	Accept(cmp, '(');

	node->_while.cond = ParseExpr(cmp, 0);

	AnalyseCondition(cmp, node->_while.cond);

	Accept(cmp, ')');

	Expect(cmp, ';');

	return node;
}

static node_t *ParseBreakStmt(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node;

	if (!cmp)
		return NULL;

	if (!cmp->lpCount && !cases)
		Error(cmp, 0, errorTable[INVALID_BRK_PLACE]);

	node = NodeNew(BREAK, cmp->file->line);

	Lex(cmp);

	Expect(cmp, ';');

	return node;
}

static node_t *ParseContinueStmt(compiler_t *cmp)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(CONTINUE, cmp->file->line);

	if (!cmp->lpCount)
		Error(cmp, 0, errorTable[INVALID_CNT_PLACE]);

	Lex(cmp);

	Expect(cmp, ';');

	return node;
}

static node_t *ParseBlock(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node, **ptr;

	if (!cmp)
		return NULL;

	node = NodeNew(BLOCK, cmp->file->line);

	ptr = &node->block.head;

	CompilerNewScope(cmp);

	cmp->flags |= CMPF_LOCAL_SCOPE;

	while (*cmp->file->src && Lex(cmp) != '}') {
		*ptr = ParseBlockStmt(cmp, cases);
		if (*ptr)
			ptr = &(*ptr)->next_stmt;
	}

	CompilerDelScope(cmp);

	Expect(cmp, '}');

	return node;
}

static node_t *ParseBlockStmt(compiler_t *cmp, hashtable_t *cases)
{
	if (!cmp || cmp->token == ';')
		return NULL;

	if (IsType(cmp) || IsStorage(cmp) || IsQualifier(cmp))
		return ParseDeclaration(cmp);

	return ParseStmt(cmp, cases);
}

static node_t *ParseIdentifier(compiler_t *cmp, hashtable_t *cases)
{
	node_t *node;
	misc_t *misc;

	if (!cmp)
		return NULL;

	cmp->flags |= CMPF_MAYBE_LABEL;

	node = ParseExpr(cmp, 0);

	if (!(cmp->flags & CMPF_MAYBE_LABEL)) {
		Expect(cmp, ';');
		return node;
	}

	cmp->flags &= ~CMPF_MAYBE_LABEL;

	if (TableGet(cmp->table[TABLE_LABELS], cmp->value.str, true))
		Error(cmp, 0, "label '%s' redefinition\n", cmp->value.str);

	node->kind = LABEL;

	misc = MiscNewLabel(cmp->value.str, 0);

	TableAdd(cmp->table[TABLE_LABELS], cmp->value.str, misc);

	node->label.lb = misc;

	if (Lex(cmp) == '}')
		return node;

	if (IsType(cmp) || IsStorage(cmp) || IsQualifier(cmp)) {
		Error(cmp, 0, errorTable[LABELED_DECL]);
		node->label.stmt = ParseDeclaration(cmp);
		return node;
	}

	node->label.stmt = ParseStmt(cmp, cases);
	return node;
}

static node_t *ParseGotoStmt(compiler_t *cmp)
{
	node_t     *node;
	const char *name;
	misc_t     *lb;

	if (!cmp)
		return NULL;

	node = NodeNew(GOTO, cmp->file->line);

	Lex(cmp);

	if (!Expect(cmp, TK_ID)) {
		Lex(cmp);
		Expect(cmp, ';');
		return node;
	}

	name = cmp->value.str;

	lb = TableGet(cmp->table[TABLE_LABELS], name, true);

	if (lb) {
		node->value = lb;
		return node;
	}

	*cmp->patcher = PatcherNew(name, &node->value, node->line);

	cmp->patcher = &(*cmp->patcher)->next;

	Lex(cmp);

	Expect(cmp, ';');

	return node;
}