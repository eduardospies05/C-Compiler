#include "compiler.h"

// ============================

static size_t tempCount;
static size_t labelCount;

static misc_t *GenIRFromTree(compiler_t *cmp, node_t *node, misc_t *l1, misc_t *l2);
static misc_t *GenLValue(compiler_t *cmp, node_t *node);
static misc_t *GenExpr(compiler_t *cmp, node_t *node);
static void    GenFunction(compiler_t *cmp, node_t *node);
static void    GenVarDecl(compiler_t *cmp, node_t *node);
static misc_t *GenMemberAccess(compiler_t *cmp, node_t *node);
static misc_t *GenMemberArrowAccess(compiler_t *cmp, node_t *node);
static misc_t *GenArrayAccess(compiler_t *cmp, node_t *node);

// ============================

void GenIR(compiler_t *cmp)
{
	if (!cmp)
		return;

	cmp->stackSubSize = 0;

	for (node_t *n = cmp->nodes; n; n = n->next_stmt)
		GenIRFromTree(cmp, n, NULL, NULL);
}

static misc_t *GenIRFromTree(compiler_t *cmp, node_t *node, misc_t *l1, misc_t *l2)
{
	if (!cmp || !node)
		return NULL;

	switch (node->kind) {
		case FUNDECL:
			GenFunction(cmp, node);
			break;
		case BLOCK:
			for (node_t *ptr = node->block.head; ptr; ptr = ptr->next_stmt)
				GenIRFromTree(cmp, ptr, l1, l2);
			break;
		case VARDECL:
			for (node_t *ptr = node; ptr; ptr = ptr->decl.next)
				GenVarDecl(cmp, ptr);
			break;
		default:
			GenExpr(cmp, node);
			break;
	}

	return NULL;
}

static void GenVarDecl(compiler_t *cmp, node_t *node)
{
	misc_t *dest, *src;

	if (!cmp || !node)
		return;

	cmp->stackSubSize = Align(cmp->stackSubSize + TypeGetSize(node->type), 4);

	node->decl.sym->value = MiscNewInt(-(int)cmp->stackSubSize);

	if (!node->decl.init)
		return;

	dest      = MiscNew(MISC_SYM);
	dest->sym = node->decl.sym;

	src = GenExpr(cmp, node->decl.init);

	AddIR(cmp, InstructionNew(IR_STORE, dest, src, NULL, node->type, node->line));
}

static misc_t *GenExpr(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node)
		return NULL;

	switch (node->kind) {
		misc_t *tmp, *lhs, *rhs;
		case IDENTIFIER:
			tmp = MiscNewTempReg(tempCount++);
			AddIR(cmp, InstructionNew(IR_COPY, tmp, node->value, NULL, node->type, node->line));
			return tmp;
		case LITERAL:
			return node->value;
		case BINARYEXPR:
			lhs = GenExpr(cmp, node->bin.lhs);
			rhs = GenExpr(cmp, node->bin.rhs);
			tmp = MiscNewTempReg(tempCount++);
			AddIR(cmp, InstructionNew(OpToKind(node->bin.op, false), tmp, lhs, rhs, node->type, node->line));
			return tmp;
		case ASSIGN:
			lhs = GenLValue(cmp, node->bin.lhs);
			rhs = GenExpr(cmp, node->bin.rhs);
			AddIR(cmp, InstructionNew(OpToKind(node->bin.op, false), lhs, rhs, NULL, node->type, node->line));
			return lhs;
		case MEMBER_ACCESS:
			tmp = GenMemberAccess(cmp, node);
			AddIR(cmp, InstructionNew(IR_LOAD, tmp, tmp, NULL, node->type, node->line));
			return tmp;
		case MEMBER_ARROW_ACCESS:
			tmp = GenMemberArrowAccess(cmp, node);
			AddIR(cmp, InstructionNew(IR_LOAD, tmp, tmp, NULL, node->type, node->line));
			return tmp;
		case ARRAYACCESS:
			tmp = GenArrayAccess(cmp, node);
			AddIR(cmp, InstructionNew(IR_LOAD, tmp, tmp, NULL, node->type, node->line));
			return tmp;
		case DEREFERENCE:
			tmp = MiscNewTempReg(tempCount++);
			AddIR(cmp, InstructionNew(IR_LOAD, tmp, GenLValue(cmp, node->unary.expr), NULL, node->type, node->line));
			return tmp;
	}

	return NULL;
}

static misc_t *GenLValue(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node)
		return NULL;

	switch (node->kind) {
		misc_t *tmp;
		case IDENTIFIER:
			return node->value;
		case MEMBER_ACCESS:
			return GenMemberAccess(cmp, node);
		case MEMBER_ARROW_ACCESS:
			return GenMemberArrowAccess(cmp, node);
		case ARRAYACCESS:
			return GenArrayAccess(cmp, node);
		case DEREFERENCE:
			tmp = MiscNewTempReg(tempCount++);
			AddIR(cmp, InstructionNew(IR_COPY, tmp, GenLValue(cmp, node->unary.expr), NULL, node->type, node->line));
			return tmp;
	}

	return NULL;
}

static misc_t *GenArrayAccess(compiler_t *cmp, node_t *node)
{
	misc_t *base, *index, *tmp;

	if (!cmp || !node)
		return NULL;

	tmp = MiscNewTempReg(tempCount++);

	index = GenExpr(cmp, node->arrayAccess.index);
	base  = GenLValue(cmp, node->arrayAccess.base);

	AddIR(cmp, InstructionNew(IR_MUL, tmp, index, MiscNewInt(TypeGetSize(node->type)), node->type, node->line));
	AddIR(cmp, InstructionNew(IR_LEA, tmp, base, tmp, node->type, node->line));

	return tmp;
}

static misc_t *GenMemberAccess(compiler_t *cmp, node_t *node)
{
	misc_t *base, *addr;

	if (!cmp || !node)
		return NULL;

	base = GenLValue(cmp, node->memberAccess.base);

	addr = MiscNewTempReg(tempCount++);

	AddIR(cmp, InstructionNew(IR_LEA, addr, base, MiscNewInt((int)node->memberAccess.member->field->offset), node->type, node->line));

	return addr;
}

static misc_t *GenMemberArrowAccess(compiler_t *cmp, node_t *node)
{
	misc_t *base, *addr, *tmp;

	if (!cmp || !node)
		return NULL;

	base = GenLValue(cmp, node->memberAccess.base);

	tmp  = MiscNewTempReg(tempCount++);

	AddIR(cmp, InstructionNew(IR_LOAD, tmp, base, NULL, node->memberAccess.base->type->base, node->line));

	addr = MiscNewTempReg(tempCount++);

	AddIR(cmp, InstructionNew(IR_LEA, addr, tmp, MiscNewInt((int)node->memberAccess.member->field->offset), node->type, node->line));

	return addr;
}

static void GenFunction(compiler_t *cmp, node_t *node)
{
	misc_t        *fn, **subEnter, **subLeave;
	int            paramOffset = 8;

	if (!cmp || !node)
		return;

	fn      = MiscNew(MISC_SYM);
	fn->sym = node->decl.sym;

	for (parameter_t *params = fn->sym->type->params; params; params = params->next) {
		int sz = (int)Align(TypeGetSize(params->type), 4);
		params->sym->value = MiscNewInt(paramOffset);
		paramOffset += sz;
	}

	AddIR(cmp, InstructionNew(IR_ENTER, fn, NULL, NULL, node->type, node->line));

	subEnter = &cmp->tail->arg2;

	tempCount = 0;

	GenIRFromTree(cmp, node->decl.init, NULL, NULL);

	AddIR(cmp, InstructionNew(IR_LEAVE, fn, NULL, NULL, node->type, node->line));

	subLeave = &cmp->tail->arg2;

	*subEnter = *subLeave = MiscNewInt(cmp->stackSubSize);

	cmp->stackSubSize = 0;
}