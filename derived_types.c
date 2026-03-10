#include "compiler.h"

// ==========================================

static size_t derivedDepth = 0;

static void CalculateDerivedOffsets(compiler_t *cmp, type_t *type, hashtable_t *table, size_t startOffset);

// ==========================================

type_t *ParseUnionStruct(compiler_t *cmp, int table)
{
	type_t     *ty;
	const char *name = NULL;
	typekind    kind = (table == TABLE_STRUCTS) ? STRUCT : UNION;
	field_t   **ptr;

	if (!cmp)
		return cmp_primitives[INT];

	if (cmp->token == TK_ID) {
		name = cmp->value.str;
		ty = TableGet(cmp->table[table], name, false);
		if (!ty)
			ty = TypeNewDerived(kind, name), TableAdd(cmp->table[table], name, ty);
		Lex(cmp);
	}
	else
		ty = TypeNewDerived(kind, NULL);

	if (cmp->token != '{')
		return ty;

	ty->hasDefinition = true;

	ptr = &ty->fields;

	Lex(cmp);

	derivedDepth++;

	do {
		type_t *base = ParseDeclaration_Level0(cmp, NULL);
		if (IsComposite(base) && cmp->token == ';') {
			*ptr = FieldNew(NULL, base);
			ptr = &(*ptr)->next;
			continue;
		}
		do {
			const char *vname = NULL;
			type_t     *ty   = ParseDeclaration_Level1(cmp, base, &vname);

			CheckIncompleteType(cmp, ty, SCOPE_LOCAL, KW_AUTO);

			*ptr = FieldNew(vname, ty);

		    ptr = &(*ptr)->next;

			if (cmp->token != ',')
				break;

		} while (Lex(cmp));
		Expect(cmp, ';');
	} while (Lex(cmp) != '}');

	Accept(cmp, '}');

	derivedDepth--;

	if (!derivedDepth)
		CalculateDerivedOffsets(cmp, ty, NULL, 0);

	return ty;
}

static void CalculateDerivedOffsets(compiler_t *cmp, type_t *type, hashtable_t *table, size_t startOffset)
{
	size_t currentOffset = startOffset;
	size_t max_align = 1, max_size = 0;

	bool isStruct;

	if (!cmp || !type)
		return;

	isStruct = IsStruct(type);

	type->members = (table) ? table : TableNew(false);

	for (field_t *field = type->fields; field; field = field->next) {
		size_t align = TypeGetAlign(field->type);

		max_align = max(max_align, align);

		if (isStruct) {
			currentOffset = Align(currentOffset, align);
			field->offset = currentOffset;
		}
		else
			field->offset = startOffset;

		if (field->name) {
			TableAdd(type->members, field->name, field);
			if (IsComposite(field->type))
				CalculateDerivedOffsets(cmp, field->type, NULL, currentOffset);
		}
		else
			CalculateDerivedOffsets(cmp, field->type, type->members, currentOffset);

		if (isStruct) {
			currentOffset += TypeGetSize(field->type);
			continue;
		}

		max_size = max(max_size, TypeGetSize(field->type));
	}

	type->align = max_align;

	if (isStruct) {
		type->size = Align(currentOffset - startOffset, max_align);
		return;
	}

	type->size = Align(max_size, max_align);
}