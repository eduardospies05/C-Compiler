#include "compiler.h"

void  AnalyseStrInit(compiler_t *cmp, node_t *node, type_t *type)
{
	size_t strLen;

	if (!cmp || !node || !type || !node->value)
		return;

	strLen = strlen(node->value->str);

	if (type->size) {
		if (strLen > type->size)
			Error(cmp, 0, "incompatible types\n");
		return;
	}

	type->size = strLen;
}

void AnalyseMemberAccess(compiler_t *cmp, node_t *node)
{
	type_t     *ty;
	const char *str;

	if (!cmp || !node || !node->memberAccess.base || !node->memberAccess.member)
		return;

	ty = DropQualifiers(node->memberAccess.base->type);

	if (!IsComposite(ty)) {
		Error(cmp, 0, errorTable[COMPOSITE_EX]);
		return;
	}

	str = node->memberAccess.member->str;

	node->memberAccess.member->field = TableGet(ty->members, str, true);

	if (!node->memberAccess.member->field) {
		Error(cmp, 0, "undefined field '%s'\n", str);
		return;
	}

	node->type = node->memberAccess.member->field->type;
}

void AnalyseMemberPtrAccess(compiler_t *cmp, node_t *node)
{
	type_t     *ty;
	const char *str;

	if (!cmp || !node || !node->memberAccess.base || !node->memberAccess.member)
		return;

	ty = DropQualifiers(node->memberAccess.base->type);

	if (!IsPointer(ty)) {
		Error(cmp, 0, errorTable[PTR_EXPECTED]);
		return;
	}

	ty = ty->base;

	if (!IsComposite(ty)) {
		Error(cmp, 0, errorTable[COMPOSITE_EX]);
		return;
	}

	str = node->memberAccess.member->str;

	node->memberAccess.member->field = TableGet(ty->members, str, true);

	if (!node->memberAccess.member->field) {
		Error(cmp, 0, "undefined field '%s'\n", str);
		return;
	}

	node->type = node->memberAccess.member->field->type;
}

void AnalyseFunctionCall(compiler_t *cmp, node_t *node)
{
	type_t *fn;
	parameter_t *p;
	node_t *pArg;

	if (!cmp || !node || !node->fncall.base)
		return;

	if (!IsFunction(node->fncall.base->type)) {
		Error(cmp, 0, errorTable[FUNCTION_EX]);
		return;
	}

	fn = DropQualifiers(node->fncall.base->type);

	if (fn->kind == PTR)
		fn = fn->base;

	node->type = fn->base;

	if (fn->hasVoidParam) {
		if (node->fncall.count)
			Error(cmp, 0, errorTable[TOO_MANY_ARGS]);
		return;
	}

	for (p = fn->params, pArg = node->fncall.args; p && pArg; p = p->next, pArg = pArg->next_stmt)
		if (!IsCompatible(p->type, pArg->type))
			Error(cmp, 0, "incompatible types\n");

	if (fn->paramCount != node->fncall.count) {
		if (node->fncall.count < fn->paramCount) {
			Error(cmp, 0, errorTable[TOO_FEW_ARGS]);
			return;
		}

		if (node->fncall.count > fn->paramCount && !fn->hasElipsis)
			Warning(cmp, 0, errorTable[TOO_MANY_ARGS]);
	}
}

void AnalyseTernary(compiler_t *cmp, node_t *node)
{
	type_t *t1, *t2;

	if (!cmp || !node || !node->ternary.cond || !node->ternary._true || !node->ternary._false)
		return;

	AnalyseCondition(cmp, node->ternary.cond);

	t1 = DropQualifiers(node->ternary._true->type);
	t2 = DropQualifiers(node->ternary._false->type);

	if (IsEqual(t1, t2)) {
		node->type = t1;
		return;
	}

	if (!IsArith(t1) || !IsArith(t2))
		Error(cmp, 0, errorTable[ARITH_EXPECTED]);

	if (IsPrimitive(t1) && IsPrimitive(t2)) {
		node->type = t1->kind > t2->kind ? node->ternary._true->type : node->ternary._false->type;
		return;
	}

	if (IsPointer(t1) || IsPointer(t2)) {
		node->type = IsPointer(t1) ? node->ternary._true->type : node->ternary._false->type;
		return;
	}
}

void AnalyseShortCircuit(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node)
		return;

	if (!IsArith(node->bin.lhs->type) || !IsArith(node->bin.rhs->type))
		Error(cmp, 0, errorTable[ARITH_EXPECTED]);
}

void AnalyseNegation(compiler_t *cmp, node_t *node)
{
	type_t *t1;

	if (!cmp || !node || !node->unary.expr)
		return;

	t1 = node->unary.expr->type;
	
	if (!IsInteger(t1))
		Error(cmp, 0, errorTable[ARITH_ONLY]);

	node->type = node->unary.expr->type;
}

void AnalyseUnaryPlusMinus(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node || !node->unary.expr)
		return;

	if (!IsPrimitive(node->unary.expr->type))
		Error(cmp, 0, errorTable[ARITH_ONLY]);

	node->type = node->unary.expr->type;
}

void AnalyseArrayAccess(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node || !node->arrayAccess.base || !node->arrayAccess.index)
		return;

	if (IsPointer(node->arrayAccess.base->type)) {
		if (!IsArith(node->arrayAccess.index->type))
			Error(cmp, 0, errorTable[ARITH_EXPECTED]);
		node->type = node->arrayAccess.base->type->base;
		return;
	}

	if (IsPointer(node->arrayAccess.index->type)) {
		node_t *tmp = node->arrayAccess.base;
		node->arrayAccess.base = node->arrayAccess.index;
		node->arrayAccess.index = tmp;
		if (!IsArith(node->arrayAccess.index->type))
			Error(cmp, 0, errorTable[ARITH_EXPECTED]);
		node->type = node->arrayAccess.base->type->base;
		return;
	}

	Error(cmp, 0, errorTable[PTR_EXPECTED]);
}

void AnalyseAddressOf(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node || !node->unary.expr)
		return;

	node->type = TypeNewPtr(node->unary.expr->type);
}

void AnalyseIncDec(compiler_t *cmp, node_t *node)
{
	type_t *ty;

	if (!cmp || !node || !node->unary.expr)
		return;

	ty = node->unary.expr->type;

	if (ty->kind == CONST)
		Error(cmp, 0, errorTable[MODIFIABLE_LVALUE_EX]);

	if (!IsArith(ty))
		Error(cmp, 0, errorTable[ARITH_EXPECTED]);

	node->type = ty;
}

void AnalyseTypecast(compiler_t *cmp, node_t *node)
{
	type_t *t1, *t2;

	if (!cmp || !node || !node->typecast.from || !node->typecast.to)
		return;

	t1 = DropQualifiers(node->typecast.to);
	t2 = DropQualifiers(node->typecast.from->type);

	if (IsComposite(t1) || IsComposite(t2))
		Error(cmp, 0, "invalid cast\n");

	node->type = t1;
}

void AnalyseDereference(compiler_t *cmp, node_t *node)
{
	type_t *ty;

	if (!cmp || !node || !node->unary.expr)
		return;

	ty = DropQualifiers(node->unary.expr->type);

	if (!IsPointer(ty)) {
		Error(cmp, 0, errorTable[PTR_EXPECTED]);
		return;
	}

	node->type = ty->base;

	CheckIncompleteType(cmp, node->type, TableGetScope(cmp->table[TABLE_SYMBOLS]), KW_AUTO);
}

void AnalyseReturn(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node)
		return;

	if (!IsCompatible(node->type, cmp->currentFnType))
		Error(cmp, 0, "incompatible types\n");

	node->type = cmp->currentFnType;
}

void AnalyseCondition(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node)
		return;

	if (!IsArith(node->type))
		Error(cmp, 0, errorTable[ARITH_EXPECTED]);
}

void AnalyseIdentifier(compiler_t *cmp, node_t *node)
{
	symbol_t *sym;

	if (!cmp || !node)
		return;

	sym = CompilerGetSymbol(cmp, node->value->str, false);

	if (!sym) {
		Error(cmp, 0, "undefined reference to identifier '%s'\n", node->value->str);
		return;
	}

	if (sym->value) {
		node->kind  = LITERAL;
		node->value = sym->value;
		node->type  = cmp_primitives[INT];
		return;
	}

	node->value->kind = MISC_SYM;
	node->value->sym = sym;
	node->type = sym->type;
}

void AnalyseVarDecl(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node || !node->decl.init)
		return;

	if (!IsCompatible(node->decl.sym->type, node->decl.init->type))
		Error(cmp, 0, "incompatible types\n");

	node->type = node->decl.sym->type;
}

void AnalyseFunction(compiler_t *cmp, node_t *node)
{
	symbol_t *fn, *proto;

	if (!cmp || !node)
		return;

	fn = node->decl.sym;

	proto = CompilerGetSymbol(cmp, fn->name, true);

	CompilerAddSymbol(cmp, fn);

	if (proto) {
		if (proto->isPrototype) {
			if (!IsEqual(fn->type, proto->type))
				Error(cmp, 0, "function '%s' signature mismatch\n", fn->name);
		}
		else
			Error(cmp, 0, "function '%s' redefinition\n", fn->name);
	}

	CompilerNewScope(cmp);

	for (parameter_t *p = fn->type->params; p; p = p->next) {
		if (!p->sym) {
			Error(cmp, 0, errorTable[UNNAMED_PARAM]);
			continue;
		}
		CompilerAddSymbol(cmp, p->sym);
	}

	node->type = fn->type;

	cmp->currentFnType = fn->type->base;
}

void AnalyseAssign(compiler_t *cmp, node_t *node)
{
	type_t *t1, *t2;

	if (!cmp || !node)
		return;

	t1 = node->bin.lhs->type, t2 = node->bin.rhs->type;

	if (t1->kind == CONST)
		Error(cmp, 0, errorTable[MODIFIABLE_LVALUE_EX]);

	if (!IsCompatible(t1, t2))
		Error(cmp, 0, "incompatible types\n");

	node->type = t1;
}

void AnalyseBinaryExpr(compiler_t *cmp, node_t *node)
{
	type_t *t1, *t2;
	bool    bothPointers;

	if (!cmp || !node)
		return;

	t1 = DropQualifiers(node->bin.lhs->type), t2 = DropQualifiers(node->bin.rhs->type);

	if (!IsArith(t1) || !IsArith(t2))
		Error(cmp, 0, errorTable[ARITH_EXPECTED]);

	if (IsPrimitive(t1) && IsPrimitive(t2)) {
		node->type = t1->kind > t2->kind ? t1 : t2;
		return;
	}

	bothPointers = IsPointer(t1) && IsPointer(t2);

	if (bothPointers && node->bin.op == '-') {
		node->type = cmp_primitives[LONG];
		return;
	}

	if (node->bin.op == '+' || node->bin.op == '-') {
		node->type = IsPointer(t1) ? t1 : t2;
		return;
	}

	Error(cmp, 0, errorTable[INTEGRAL_EXPECTED]);
}

void AnalyseLiteral(compiler_t *cmp, node_t *node)
{
	int64_t i;
	uint64_t u;

	if (!cmp || !node)
		return;

	i = node->value->i;
	u = (uint64_t)i;

	if (i >= INT_MIN && i <= INT_MAX)
		return;

	if (u <= UINT_MAX && i >= 0) {
		node->type = cmp_primitives[UINT];
		return;
	}

	if (i >= LLONG_MIN && i <= LLONG_MAX) {
		node->type = cmp_primitives[LONG];
		return;
	}

	node->type = cmp_primitives[ULONG];
}