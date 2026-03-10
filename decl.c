#include "compiler.h"

// ==========================================

static type_t *ParseFunctionParams(compiler_t *cmp, type_t *base);
static type_t *ParseArray(compiler_t *cmp, type_t *base);
static node_t *ParseFunction(compiler_t *cmp, symbol_t *fn);
static void    HandleTypedef(compiler_t *cmp, const char *name, type_t *type);

static node_t *ParseInitializer(compiler_t *cmp, type_t *type);
static node_t *ParseArrayInitializer(compiler_t *cmp, type_t *type);

static void    PatchLabels(compiler_t *cmp, patcher_t *patches);

// ==========================================

node_t *ParseTranslationUnit(compiler_t *cmp)
{
	if (!cmp)
		return NULL;

	return ParseDeclaration(cmp);
}

node_t *ParseDeclaration(compiler_t *cmp)
{
	node_t *decl = NULL, **ptr = &decl;
	size_t  storage = 0;
	type_t *base;
	const char *name = NULL;

	if (!cmp)
		return NULL;

	base = ParseDeclaration_Level0(cmp, &storage);

	if ((IsComposite(base) || IsEnum(base)) && cmp->token == ';')
		return NULL;

	do {
		type_t   *type = ParseDeclaration_Level1(cmp, base, &name);
		size_t    scope = TableGetScope(cmp->table[TABLE_SYMBOLS]);
		symbol_t *sym;

		cmp->flags &= ~CMPF_IGNORE_ID;

		if (storage == KW_TYPEDEF) {
			HandleTypedef(cmp, name, type);
			if (cmp->token != ',')
				break;
			continue;
		}

		sym = SymbolNew(name, type, scope, storage, cmp->file->line);

		if (type->kind == FUNCTION)
			return ParseFunction(cmp, sym);
		
		*ptr = NodeNew(VARDECL, cmp->file->line);
		(*ptr)->type = type;

		(*ptr)->decl.sym = sym;

		CompilerAddSymbol(cmp, (*ptr)->decl.sym);

		if (cmp->token == '=') {
			Lex(cmp);
			(*ptr)->decl.init = ParseInitializer(cmp, type);
			AnalyseVarDecl(cmp, *ptr);
		}

		CheckIncompleteType(cmp, type, scope, storage);

		if (cmp->token != ',')
			break;

		ptr = &(*ptr)->decl.next;
	} while (Lex(cmp));

	Expect(cmp, ';');

	return decl;
}

static node_t *ParseFunction(compiler_t *cmp, symbol_t *fn)
{
	node_t    *node;
	patcher_t *patch = NULL;

	if (!cmp || !fn)
		return NULL;

	if (cmp->token == ';') {
		fn->isPrototype = true;
		CompilerAddSymbol(cmp, fn);
		return NULL;
	}

	node = NodeNew(FUNDECL, cmp->file->line);

	node->decl.sym = fn;

	Expect(cmp, '{');

	AnalyseFunction(cmp, node);

	cmp->patcher = &patch;

	cmp->table[TABLE_LABELS] = TableNew(false);

	node->decl.init = ParseStmt(cmp, NULL);

	PatchLabels(cmp, patch);

	CompilerDelScope(cmp);

	return node;
}

type_t *ParseDeclaration_Level1(compiler_t *cmp, type_t *base, const char **name)
{
	type_t *final = base;

	if (!cmp || !base || !name)
		return cmp_primitives[INT];

	while (cmp->token == '*') {
		final = TypeNewPtr(final);
		Lex(cmp);
		if (IsQualifier(cmp)) {
			type_t *tmp = TypeNew(cmp->token == KW_CONST ? CONST : VOLATILE);
			tmp->base = final, final = tmp;
			Lex(cmp);
		}
	}

	if (cmp->token == '(') {
		type_t *prefix, *suffix, **ptr;

		Lex(cmp);

		prefix = ParseDeclaration_Level1(cmp, base, name);

		Accept(cmp, ')');
		
		suffix = ParseDeclaration_Level2(cmp, final);

		for (ptr = &prefix; *ptr != base; ptr = &(*ptr)->base);

		*ptr = suffix;

		return prefix;
	}

	if (cmp->token == TK_ID || !(cmp->flags & CMPF_IGNORE_ID)) {
		Expect(cmp, TK_ID);
		if (cmp->token == TK_ID)
			*name = cmp->value.str;
		Lex(cmp);
	}

	return ParseDeclaration_Level2(cmp, final);
}

type_t *ParseDeclaration_Level2(compiler_t *cmp, type_t *base)
{
	if (!cmp || !base)
		return cmp_primitives[INT];

	if (cmp->token == '(')
		return ParseFunctionParams(cmp, base);
	if (cmp->token == '[')
		return ParseArray(cmp, base);

	return base;
}

static type_t *ParseArray(compiler_t *cmp, type_t *base)
{
	type_t *array = NULL, **ptr;
	long    length;
	if (!cmp || !base)
		return cmp_primitives[INT];

	ptr = &array;

	do {
		if (Lex(cmp) == ']') {
			*ptr = TypeNew(ARRAY);
			ptr = &(*ptr)->base;
			continue;
		}

		length = SolveExpr(cmp, ParseExpr(cmp, 0));

		if (length < 0)
			Error(cmp, 0, errorTable[ARRAY_INVALID_LENGTH]);

		*ptr          = TypeNew(ARRAY);
		(*ptr)->size  = length;
		(*ptr)->align = base->align;
		ptr = &(*ptr)->base;

		Expect(cmp, ']');
	} while (Lex(cmp) == '[');

	*ptr = base;

	return array;
}

static type_t *ParseFunctionParams(compiler_t *cmp, type_t *base)
{
	parameter_t *params = NULL, **ptr = &params;
	const char *name = NULL;
	size_t pCount = 0;
	bool elipsis = false, hasVoid = false;

	if (!cmp || !base)
		return cmp_primitives[INT];

	if (Lex(cmp) == ')') {
		type_t *ty = TypeNewFunction(base, 0, false, false, NULL);
		Lex(cmp);
		return ty;
	}

	cmp->flags |= CMPF_IGNORE_ID;

	do {
		type_t *type, *tmp;

		if (cmp->token == TK_ELIPSIS && pCount) {
			elipsis = true;
			Lex(cmp);
			break;
		}

		type = ParseDeclaration_Level1(cmp, ParseDeclaration_Level0(cmp, NULL), &name);

		if (type->kind == VOID && !pCount) {
			hasVoid = true;
			break;
		}

		tmp = DropQualifiers(type);

		if (tmp->kind == ARRAY) {
			tmp->kind  = PTR;
			tmp->size = sizeof(void *);
			tmp->align = sizeof(void *);
		}

		pCount++;

		CheckIncompleteType(cmp, type, SCOPE_PARAM, 0);

		*ptr = ParameterNew(NULL, type);

		if (name) {
			(*ptr)->sym = SymbolNew(name, type, SCOPE_PARAM, KW_AUTO, cmp->file->line);
			name = NULL;
		}

		ptr = &(*ptr)->next;

		if (cmp->token != ',')
			break;

	} while (Lex(cmp));

	Accept(cmp, ')');

	return TypeNewFunction(base, pCount, hasVoid, elipsis, params);
}

type_t *ParseDeclaration_Level0(compiler_t *cmp, size_t *sclass)
{
	type_t *ty = cmp_primitives[INT], *qualifiers = NULL, **ptr = &qualifiers;
	size_t sum = 0;
	const char *userType = NULL;

	if (!cmp)
		return ty;

	while (IsType(cmp) || IsQualifier(cmp) || IsStorage(cmp)) {
		switch (cmp->token) {
			case KW_CONST:
			case KW_VOLATILE:
				*ptr = TypeNew(cmp->token == KW_CONST ? CONST : VOLATILE);
				ptr = &(*ptr)->base;
				break;
			case KW_VOID:
				sum += MASK_VOID;
				break;
			case KW_SIGNED:
				sum += MASK_SIGNED;
				break;
			case KW_UNSIGNED:
				sum += MASK_UNSIGNED;
				break;
			case KW_CHAR:
				sum += MASK_CHAR;
				break;
			case KW_SHORT:
				sum += MASK_SHORT;
				break;
			case KW_INT:
				sum += MASK_INT;
				break;
			case KW_LONG:
				sum += MASK_LONG;
				break;
			case KW_FLOAT:
				sum += MASK_FLOAT;
				break;
			case KW_DOUBLE:
				sum += MASK_DOUBLE;
				break;
			case TK_ID:
				sum += MASK_ID;
				userType = cmp->value.str;
				break;
			case KW_STRUCT:
				sum += MASK_STRUCT;
				break;
			case KW_UNION:
				sum += MASK_UNION;
				break;
			case KW_ENUM:
				sum += MASK_ENUM;
				break;
			case KW_STATIC:
			case KW_EXTERN:
			case KW_TYPEDEF:
			case KW_REGISTER:
			case KW_AUTO:
				if (!sclass) {
					Error(cmp, 0, "storage class not allowed here\n");
					break;
				}

				if (*sclass) {
					Error(cmp, 0, "storage class already specified\n");
					break;
				}

				*sclass += cmp->token;
				break;
		}
		Lex(cmp);
	}

	switch (sum) {
		case MASK_VOID:
			ty = cmp_primitives[VOID];
			break;
		case MASK_CHAR:
		case MASK_SIGNED + MASK_CHAR:
			ty = cmp_primitives[CHAR];
			break;
		case MASK_UNSIGNED + MASK_CHAR:
			ty = cmp_primitives[UCHAR];
			break;
		case MASK_SHORT:
		case MASK_SHORT  + MASK_INT:
		case MASK_SIGNED + MASK_SHORT:
		case MASK_SIGNED + MASK_SHORT + MASK_INT:
			ty = cmp_primitives[SHORT];
			break;
		case MASK_UNSIGNED + MASK_SHORT:
		case MASK_UNSIGNED + MASK_SHORT + MASK_INT:
			ty = cmp_primitives[USHORT];
			break;
		case MASK_INT:
		case MASK_SIGNED + MASK_INT:
		case MASK_SIGNED:
		case 0:
			break;
		case MASK_UNSIGNED:
		case MASK_UNSIGNED + MASK_INT:
			ty = cmp_primitives[UINT];
			break;
		case MASK_LONG:
		case MASK_LONG   + MASK_INT:
		case MASK_LONG   + MASK_LONG:
		case MASK_LONG   + MASK_LONG + MASK_INT:
		case MASK_SIGNED + MASK_LONG:
		case MASK_SIGNED + MASK_LONG + MASK_INT:
		case MASK_SIGNED + MASK_LONG + MASK_LONG:
		case MASK_SIGNED + MASK_LONG + MASK_LONG + MASK_INT:
			ty = cmp_primitives[LONG];
			break;
		case MASK_UNSIGNED + MASK_LONG:
		case MASK_UNSIGNED + MASK_LONG + MASK_INT:
		case MASK_UNSIGNED + MASK_LONG + MASK_LONG:
		case MASK_UNSIGNED + MASK_LONG + MASK_LONG + MASK_INT:
			ty = cmp_primitives[ULONG];
			break;
		case MASK_FLOAT:
			ty = cmp_primitives[FLOAT];
			break;
		case MASK_DOUBLE:
			ty = cmp_primitives[DOUBLE];
			break;
		case MASK_LONG + MASK_DOUBLE:
			ty = cmp_primitives[LDOUBLE];
			break;
		case MASK_ID:
			ty = TableGet(cmp->table[TABLE_TYPEDEFS], userType, false);
			break;
		case MASK_STRUCT:
			ty = ParseUnionStruct(cmp, TABLE_STRUCTS);
			break;
		case MASK_UNION:
			ty = ParseUnionStruct(cmp, TABLE_UNIONS);
			break;
		case MASK_ENUM:
			ty = ParseEnum(cmp);
			break;
		default:
			Error(cmp, 0, "invalid declaration\n");
			break;
	}

	if (qualifiers)
		*ptr = ty, ty = qualifiers;

	return ty;
}

static void HandleTypedef(compiler_t *cmp, const char *name, type_t *type)
{
	type_t *tmp;

	if (!cmp || !name || !type)
		return;

	if ((tmp = TableGet(cmp->table[TABLE_TYPEDEFS], name, true)) && tmp != type) {
		Error(cmp, 0, "symbol '%s' redefinition\n", name);
		return;
	}

	TableAdd(cmp->table[TABLE_TYPEDEFS], name, type);
}

static void PatchLabels(compiler_t *cmp, patcher_t *patches)
{
	if (!cmp || !patches)
		return;

	while (patches) {
		*patches->data = TableGet(cmp->table[TABLE_LABELS], patches->name, true);
		if (!*patches->data)
			Error(cmp, patches->line, "label '%s' not declared\n", patches->name);
		patches = patches->next;
	}
}

static node_t *ParseInitializer(compiler_t *cmp, type_t *type)
{
	node_t *node;

	if (!cmp || !type)
		return NULL;

	if (cmp->token == '{') {
		node = ParseArrayInitializer(cmp, type);
		if (node && type->kind != ARRAY && node->arrayInit.count > 1)
			Error(cmp, 0, errorTable[TOO_MANY_VALUES]);
		return node;
	}

	node = ParseExpr(cmp, opPrec[',']);

	if (node && node->kind == STRING && type->kind == ARRAY)
		AnalyseStrInit(cmp, node, type);

	return node;
}

static node_t *ParseArrayInitializer(compiler_t *cmp, type_t *type)
{
	node_t *node, **ptr;
	type_t *elemType;

	if (!cmp || !type)
		return NULL;

	node = NodeNew(ARRAY_INITIALIZER, cmp->file->line);

	node->type = type;

	elemType = (type->kind == ARRAY) ? type->base : type;

	ptr = &node->arrayInit.head;

	if (Lex(cmp) == '}') {
		Error(cmp, 0, errorTable[ARRAY_MISSING_INIT]);
		return NULL;
	}

	do {
		if (cmp->token != '[')
			*ptr = ParseInitializer(cmp, elemType);
		else {
			long num;

			*ptr = NodeNew(ARRAY_SETPOS_INIT, cmp->file->line);

			Lex(cmp);

			num = SolveExpr(cmp, ParseExpr(cmp, 0));

			(*ptr)->arrayPosInit.pos_num = num;

			if (num < 0)
				Error(cmp, 0, errorTable[ARRAY_INVALID_LENGTH]);

			Accept(cmp, ']');
			Accept(cmp, '=');

			(*ptr)->arrayPosInit.value = ParseInitializer(cmp, elemType);
			if ((*ptr)->arrayPosInit.value)
				(*ptr)->type = (*ptr)->arrayPosInit.value->type;
		}

		if (!IsCompatible((*ptr)->type, elemType))
			Error(cmp, 0, "incompatible types\n");

		node->arrayInit.count++;

		if (cmp->token != ',')
			break;

		if (Lex(cmp) == '}')
			break;

		ptr = &(*ptr)->next_stmt;
	} while (true);

	Accept(cmp, '}');

	if (type->kind == ARRAY) {
		if (!type->size) {
			type->size = node->arrayInit.count;
			return node;
		}
		if (node->arrayInit.count > type->size) {
			Error(cmp, 0, errorTable[TOO_MANY_VALUES]);
			return node;
		}
	}

	return node;
}

type_t *ParseType(compiler_t *cmp)
{
	const char *name = NULL;
	type_t *type;

	if (!cmp)
		return cmp_primitives[INT];

	cmp->flags |= CMPF_IGNORE_ID;

	type = ParseDeclaration_Level1(cmp, ParseDeclaration_Level0(cmp, NULL), &name);

	cmp->flags &= ~CMPF_IGNORE_ID;

	if (name)
		Error(cmp, 0, "cannot have an identifier here\n");

	return type;
}