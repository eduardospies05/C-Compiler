#include "compiler.h"

bool IsStruct(type_t *type)
{
	if (!type)
		return false;

	type = DropQualifiers(type);

	return type->kind == STRUCT;
}

bool IsUnion(type_t *type)
{
	if (!type)
		return false;

	type = DropQualifiers(type);
	
	return type->kind == UNION;
}

void CheckIncompleteType(compiler_t *cmp, type_t *type, size_t scope, size_t storage)
{
	if (!cmp || !type)
		return;

	type = DropQualifiers(type);

	if (type->kind == ARRAY && !type->size) {
		if ((scope == SCOPE_PARAM || storage == KW_EXTERN) && (type->base->kind != ARRAY))
			return;
		Error(cmp, 0, errorTable[INCOMPLETE_TYPE]);
		return;
	}

	if (type->kind == VOID || (type->kind == ARRAY && type->base->kind == VOID) || 
	   (IsComposite(type) && !type->hasDefinition))
		Error(cmp, 0, errorTable[INCOMPLETE_TYPE]);
}

bool IsEqual(type_t *t1, type_t *t2)
{
	if (!t1 || !t2)
		return false;

	if (t1->kind != t2->kind && (IsPointer(t1) && IsPointer(t2)))
		t1 = t1->base, t2 = t2->base;

	t1 = DropQualifiers(t1);
	t2 = DropQualifiers(t2);

	if (t1->kind != t2->kind)
		return false;

	switch (t1->kind) {
		case ARRAY:
		case PTR:
			return IsEqual(t1->base, t2->base);
		case FUNCTION:
			if (t1->paramCount != t2->paramCount || (t1->hasVoidParam != t2->hasVoidParam || t1->hasElipsis != t2->hasElipsis))
				return false;
			for (parameter_t *p1 = t1->params, *p2 = t2->params; p1 && p2; p1 = p1->next, p2 = p2->next)
				if (!IsEqual(p1->type, p2->type))
					return false;
			return IsEqual(t1->base, t2->base);
		default:
			return true;
	}
}

bool IsCompatible(type_t *t1, type_t *t2)
{
	if (!t1 || !t2)
		return false;

	t1 = DropQualifiers(t1);
	t2 = DropQualifiers(t2);

	if (IsPrimitive(t1) && IsPrimitive(t2))
		return true;

	if (IsPointer(t1) && IsPointer(t2)) {
		if (t1->base->kind == VOID || t2->base->kind == VOID)
			return true;
		return IsEqual(t1, t2);
	}

	if (IsPointer(t1) && IsInteger(t2) ||
		IsInteger(t1) && IsPointer(t2))
		return true;

	if (IsFunction(t1) && IsPointer(t2))
		return IsEqual(t1, t2->base);
	if (IsPointer(t1) && IsFunction(t2))
		return IsEqual(t1->base, t2);

	return IsEqual(t1, t2);
}

bool IsArith(type_t *type)
{
	if (!type)
		return false;

	return IsPrimitive(type) || IsPointer(type) || IsEnum(type);
}

bool IsPrimitive(type_t *type)
{
	if (!type)
		return false;

	type = DropQualifiers(type);

	return type->kind >= CHAR && type->kind <= LDOUBLE;
}

bool IsPointer(type_t *type)
{
	if (!type)
		return false;

	type = DropQualifiers(type);

	return type->kind == PTR || type->kind == ARRAY;
}

bool IsLeftValue(node_t *node)
{
	if (!node)
		return false;

	switch (node->kind) {
		case MEMBER_ACCESS:
		case MEMBER_ARROW_ACCESS:
		case DEREFERENCE:
		case IDENTIFIER:
			return true;
		default:
			return false;
	}
}

type_t *DropQualifiers(type_t *type)
{
	if (!type)
		return cmp_primitives[INT];

	while (type && type->base && (type->kind == CONST || type->kind == VOLATILE))
		type = type->base;

	return type;
}

type_t *GetBaseType(type_t *type)
{
	if (!type)
		return cmp_primitives[INT];

	type = DropQualifiers(type);

	if (type->kind == ARRAY)
		return GetBaseType(type->base);

	return type;
}

size_t TypeGetAlign(type_t *type)
{
	if (!type)
		return 0;

	type = DropQualifiers(type);

	if (IsComposite(type) && !type->align) {
		size_t max_align = 1;
		for (field_t *f = type->fields; f; f = f->next)
			max_align = max(max_align, TypeGetAlign(f->type));

		return max_align;
	}

	return type->align;
}

bool IsFunction(type_t *t1)
{
	if (!t1)
		return false;

	t1 = DropQualifiers(t1);

	return t1->kind == FUNCTION || (t1->kind == PTR && t1->base->kind == FUNCTION);
}

bool IsInteger(type_t *type)
{
	if (!type)
		return false;

	type = DropQualifiers(type);

	return type->kind > VOID && type->kind < FLOAT;
}

bool IsComposite(type_t *type)
{
	if (!type)
		return false;

	type = DropQualifiers(type);

	return type->kind == STRUCT || type->kind == UNION;
}

bool IsEnum(type_t *type)
{
	if (!type)
		return false;

	type = DropQualifiers(type);

	return type->kind == ENUM;
}