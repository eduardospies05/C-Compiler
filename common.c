#include "compiler.h"

// ==========================================

static void Msg(compiler_t *cmp, const char *msg, ...);
static int SolveBinaryExpr(compiler_t *cmp, node_t *node);

// ==========================================

misc_t *MiscNewLabel(const char *opt_name, size_t opt_id)
{
	misc_t *misc;

	misc = Alloc(sizeof(*misc) + sizeof(label_t));

	misc->kind = MISC_LABEL;

	misc->lb = (label_t *)(misc + 1);

	misc->lb->name = opt_name;
	misc->lb->id   = opt_id;
	misc->lb->addr = 0;

	return misc;
}

misc_t *MiscNewTempReg(size_t id)
{
	misc_t *misc;

	misc = Alloc(sizeof(*misc) + sizeof(temporary_t));

	misc->kind = MISC_TEMP;

	misc->reg = (temporary_t *)(misc + 1);

	misc->reg->id = id;
	misc->reg->real_reg = -1;

	return misc;
}

misc_t *MiscNewInt(int value)
{
	misc_t *m;

	m = MiscNew(MISC_INT);

	m->i = value;

	return m;
}

patcher_t *PatcherNew(const char *name, misc_t **data, size_t line)
{
	patcher_t *patch;

	patch = Alloc(sizeof(*patch));

	patch->name = name;
	patch->data = data;
	patch->line = line;
	patch->next = NULL;

	return patch;
}

asm_operand_t *ASM_OperandNew(asmkind kind)
{
	asm_operand_t *op;

	op = Alloc(sizeof(*op));

	memset(op, 0, sizeof(*op));

	op->kind = kind;

	return op;
}

asm_instruction_t *ASM_InstructionNew(int opcode, asm_operand_t *dest, asm_operand_t *src, asm_operand_t *ex, byte size, size_t line)
{
	asm_instruction_t *ins;

	ins = IAlloc(sizeof(*ins), ARENA_IR);

	ins->opcode = opcode;
	ins->dest = dest;
	ins->src = src;
	ins->ex = ex;
	ins->line = line;
	ins->size = size;

	return ins;
}

instruction_t *InstructionNew(instructionkind kind, misc_t *dest, misc_t *arg1, misc_t *arg2, type_t *type, size_t line)
{
	instruction_t *ins;

	ins = IAlloc(sizeof(*ins), ARENA_IR);

	memset(ins, 0, sizeof(*ins));

	ins->kind = kind;
	ins->dest = dest;
	ins->arg1 = arg1;
	ins->arg2 = arg2;
	ins->type = type ? type : cmp_primitives[INT];
	ins->line = line;

	return ins;
}

void AddIR(compiler_t *cmp, instruction_t *ins)
{
	if (!cmp || !ins)
		return;

	if (!cmp->head) {
		cmp->head = ins;
		cmp->tail = ins;
		return;
	}

	ins->prev = cmp->tail;

	cmp->tail->next = ins;
	cmp->tail = ins;
}

size_t Align(size_t a, size_t b)
{
	return (a + (b - 1)) & ~(b - 1);
}

bool IStrCmp(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return false;

	while (*s1 && *s2)
		if ((*s1++ & 0xDF) != (*s2++ & 0xDF))
			return false;

	return !*s1 && !*s2;
}

bool IsType(compiler_t *cmp)
{
	if (!cmp)
		return false;

	if (cmp->token == TK_ID)
		return TableGet(cmp->table[TABLE_TYPEDEFS], cmp->value.str, false) ? true : false;

	return cmp->token >= KW_VOID && cmp->token <= KW_ENUM;
}

bool IsQualifier(compiler_t *cmp)
{
	if (!cmp || cmp->token < KW_CONST || cmp->token > KW_VOLATILE)
		return false;
	return true;
}

bool IsStorage(compiler_t *cmp)
{
	if (!cmp)
		return false;

	return cmp->token >= KW_TYPEDEF && cmp->token <= KW_REGISTER;
}

symbol_t *SymbolNew(const char *name, type_t *type, size_t scope, size_t storage, size_t line)
{
	symbol_t *sym;

	if (!name || !type)
		return NULL;

	sym = Alloc(sizeof(*sym));

	memset(sym, 0, sizeof(*sym));

	sym->name = name;
	sym->type = type;
	sym->scope = scope;
	sym->storage = storage;
	sym->line = line;

	return sym;
}

parameter_t *ParameterNew(symbol_t *sym, type_t *type)
{
	parameter_t *param;

	if (!type)
		return NULL;

	param = Alloc(sizeof(*param));

	param->sym = sym;
	param->type = type;
	param->next = NULL;

	return param;
}

type_t* TypeNew(typekind kind)
{
	type_t* ty;

	ty = Alloc(sizeof(*ty));

	memset(ty, 0, sizeof(*ty));

	ty->kind = kind;

	return ty;
}

type_t *TypeNewFunction(type_t *ret, size_t paramCount, bool hasVoidParam, bool hasElipsis, parameter_t *params)
{
	type_t *ty;

	if (!ret)
		return NULL;

	ty = TypeNew(FUNCTION);

	ty->base = ret;
	ty->params = params;
	ty->paramCount = paramCount;
	ty->hasVoidParam = hasVoidParam;
	ty->hasElipsis = hasElipsis;
	ty->size = sizeof(void *);
	ty->align = sizeof(void *);

	return ty;
}

type_t *TypeNewPtr(type_t *base)
{
	type_t *ty;

	if (!base)
		return NULL;

	ty = TypeNew(PTR);

	ty->base  = base;
	ty->size  = sizeof(void *);
	ty->align = sizeof(void *);

	return ty;
}

type_t *TypeNewArray(type_t *base, size_t length)
{
	type_t *type;

	if (!base)
		return cmp_primitives[INT];

	type = TypeNew(ARRAY);

	type->base = base;
	type->size = length;
	type->align = base->align;

	return type;
}

type_t *TypeNewDerived(typekind kind, const char *name)
{
	type_t *ty;

	ty = TypeNew(kind);

	ty->tyname = name;

	return ty;
}

size_t TypeGetSize(type_t *type)
{
	if (!type)
		return 0;

	type = DropQualifiers(type);

	if (type->kind == ARRAY)
		return type->size * TypeGetSize(type->base);

	return type->size;
}

field_t *FieldNew(const char *name, type_t *type)
{
	field_t *field;

	if (!type)
		return NULL;

	field = Alloc(sizeof(*field));

	memset(field, 0, sizeof(*field));

	field->type = type;
	field->name = name;

	return field;
}

static void Msg(compiler_t *cmp, const char *msg, va_list ap)
{
	if (!cmp || !msg)
		return;

	vfprintf(stderr, msg, ap);
}

void Error(compiler_t *cmp, size_t opt_line, const char *msg, ...)
{
	va_list ap;

	if (!cmp || !msg)
		return;

	va_start(ap, msg);

	fprintf(stderr, "Error('%s', %i'): ", cmp->file->path, opt_line ? opt_line : cmp->file->line);

	Msg(cmp, msg, ap);

	va_end(ap);

	cmp->file->errors++;

	cmp->flags |= CMPF_ERROR;
}

void Warning(compiler_t *cmp, size_t opt_line, const char *msg, ...)
{
	va_list ap;

	if (!cmp || !msg || cmp->flags & CMPF_DISABLE_WARNINGS)
		return;

	va_start(ap, msg);

	fprintf(stderr, "Warning('%s', %i'): ", cmp->file->path, opt_line ? opt_line : cmp->file->line);

	Msg(cmp, msg, ap);

	va_end(ap);

	cmp->file->warnings++;
}

misc_t *MiscNew(misckind kind)
{
	misc_t *misc;

	misc = Alloc(sizeof(*misc));

	memset(misc, 0, sizeof(*misc));

	misc->kind = kind;

	return misc;
}

misc_t *MiscDuplicate(misc_t *obj)
{
	misc_t *misc;

	if (!obj)
		return NULL;

	misc = Alloc(sizeof(*misc));

	*misc = *obj;

	return misc;
}

node_t *NodeNew(nodekind kind, size_t line)
{
	node_t *node;

	node = IAlloc(sizeof(*node), ARENA_AST);

	memset(node, 0, sizeof(*node));

	node->kind = kind;
	node->line = line;
	node->type = cmp_primitives[INT];

	return node;
}

bool Expect(compiler_t *cmp, int tokenex)
{
	if (!cmp)
		return false;

	if (cmp->token == tokenex)
		return true;

	if (tokenex == TK_ID)
		return Error(cmp, 0, "identifier expected\n"), false;

	return Error(cmp, 0, "'%c' expected\n", (char)tokenex), false;
}

void Accept(compiler_t *cmp, int tokenex)
{
	if (!cmp || !Expect(cmp, tokenex))
		return;

	Lex(cmp);
}

void CompilerShutdown(const char *error)
{
	int status = EXIT_SUCCESS;

	if (error) {
		fprintf(stderr, error);
		status = EXIT_FAILURE;
	}

	DestroyAll();
	fgetc(stdin);
	exit(status);
}

int SolveExpr(compiler_t *cmp, node_t *node)
{
	if (!node)
		return 0;

	switch (node->kind) {
		case LITERAL:
			if (node->value->kind != MISC_INT)
				Error(cmp, 0, errorTable[INTEGRAL_EXPECTED]);
			return node->value->i;
		case UNARY_MINUS:
			return -SolveExpr(cmp, node->unary.expr);
		case UNARY_PLUS:
			return +SolveExpr(cmp, node->unary.expr);
		case BINARYEXPR:
			return SolveBinaryExpr(cmp, node);
	}

	Error(cmp, 0, errorTable[ARITH_EXPECTED]);

	return 0;
}

static int SolveBinaryExpr(compiler_t *cmp, node_t *node)
{
	int lhs, rhs;

	if (!cmp || !node)
		return 0;

	lhs = SolveExpr(cmp, node->bin.lhs);
	rhs = SolveExpr(cmp, node->bin.rhs);

	switch (node->bin.op) {
		case '+':
			return lhs + rhs;
		case '-':
			return lhs - rhs;
		case '*':
			return lhs * rhs;
		case '/':
			if (!rhs)
				Error(cmp, 0, errorTable[DIVISION_BY_ZERO]), rhs = 1;
			return lhs / rhs;
		case TK_SHR:
			return lhs >> rhs;
		case TK_SHL:
			return lhs << rhs;
		default:
			Error(cmp, 0, "invalid operation\n");
			return 0;
	}
}

string_concat_t *StrConcatNew(const char *str)
{
	string_concat_t *sc;

	if (!str)
		return NULL;

	sc = Alloc(sizeof(*sc));

	sc->str = str;
	sc->next = NULL;

	return sc;
}

instructionkind OpToKind(int op, bool memoryRef)
{
	switch (op) {
		case '+':
			return IR_ADD;
		case '-':
			return IR_SUB;
		case '*':
			return IR_MUL;
		case '/':
			return IR_DIV;
		case '=':
			return IR_STORE;
		case '^':
			return IR_XOR;
		case '&':
			return IR_AND;
		case '|':
			return IR_OR;
		case TK_SHL:
			return IR_SHL;
		case TK_SHR:
			return IR_SHR;
		case '>':
			return IR_GT;
		case '<':
			return IR_LT;
		case TK_GE:
			return IR_GE;
		case TK_LE:
			return IR_LE;
		case TK_EQUEQU:
			return IR_EQU;
		case TK_NOTEQU:
			return IR_NOTEQU;
	}

	return IR_ADD;
}