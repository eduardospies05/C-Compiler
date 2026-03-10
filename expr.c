#include "compiler.h"

// ==========================================

static node_t *ParseUnary(compiler_t *cmp, nodekind kind);
static node_t *ParseSizeof(compiler_t *cmp);
static node_t *ParseTypecast(compiler_t *cmp);
static node_t *ParsePrefix(compiler_t *cmp);
static node_t *ParsePostfix(compiler_t *cmp, node_t *base);
static node_t *NewPostfixPPMM(compiler_t *cmp, node_t *base, nodekind kind);
static node_t *ParseArrayAccess(compiler_t *cmp, node_t *base);
static node_t *ParseFunCall(compiler_t *cmp, node_t *base);
static node_t *ParseMemberAccess(compiler_t *cmp, node_t *base);
static node_t *ParseTernary(compiler_t *cmp, node_t *node);

static node_t *NewBinaryExpr(compiler_t *cmp, node_t *lhs, nodekind kind, int op, int prec);

static node_t *ParseExprNode(compiler_t *cmp, node_t *lhs);

static node_t *NewLiteral(compiler_t *cmp, type_t *type);
static node_t *ParseString(compiler_t *cmp);

// ==========================================

node_t *ParseExpr(compiler_t *cmp, int power)
{
	node_t *lhs;

	if (!cmp)
		return NULL;

	lhs = ParsePrefix(cmp);

	while (*cmp->file->src && cmp->token < KEYWORD && power < opPrec[cmp->token])
		lhs = ParseExprNode(cmp, lhs);

	return lhs;
}

static node_t *ParsePrefix(compiler_t *cmp)
{
	node_t *node = NULL;

	if (!cmp)
		return NULL;

	switch (cmp->token) {
		case TK_INT:
			node = NewLiteral(cmp, cmp_primitives[INT]);
			AnalyseLiteral(cmp, node);
			Lex(cmp);
			break;
		case TK_FLOAT:
			node = NewLiteral(cmp, cmp_primitives[FLOAT]);
			Lex(cmp);
			break;
		case TK_UNSIGNED:
			node = NewLiteral(cmp, cmp_primitives[UINT]);
			Lex(cmp);
			break;
		case TK_LONG:
			node = NewLiteral(cmp, cmp_primitives[LONG]);
			Lex(cmp);
			break;
		case TK_UNSIGNED_LONG:
			node = NewLiteral(cmp, cmp_primitives[ULONG]);
			Lex(cmp);
			break;
		case TK_ID:
			node = NodeNew(IDENTIFIER, cmp->file->line);
			if (cmp->flags & CMPF_MAYBE_LABEL) {
				if (Lex(cmp) == ':')
					return node;
				cmp->flags |= CMPF_DONT_LEX;
				cmp->flags &= ~CMPF_MAYBE_LABEL;
			}
			node->value = MiscDuplicate(&cmp->value);
			AnalyseIdentifier(cmp, node);
			Lex(cmp);
			break;
		case TK_STRING:
			node = ParseString(cmp);
			break;
		case '&':
			node = ParseUnary(cmp, ADDRESSOF);
			AnalyseAddressOf(cmp, node);
			break;
		case '*':
			node = ParseUnary(cmp, DEREFERENCE);
			AnalyseDereference(cmp, node);
			break;
		case KW_SIZEOF:
			node = ParseSizeof(cmp);
			break;
		case '!':
			node = ParseUnary(cmp, NOT);
			AnalyseCondition(cmp, node);
			break;
		case '-':
			node = ParseUnary(cmp, UNARY_MINUS);
			AnalyseUnaryPlusMinus(cmp, node);
			break;
		case '+':
			node = ParseUnary(cmp, UNARY_PLUS);
			AnalyseUnaryPlusMinus(cmp, node);
			break;
		case '~':
			node = ParseUnary(cmp, NEGATION);
			AnalyseNegation(cmp, node);
			break;
		case '(':
			Lex(cmp);
			if (IsType(cmp) || IsQualifier(cmp) || IsStorage(cmp)) {
				node = ParseTypecast(cmp);
				AnalyseTypecast(cmp, node);
				break;
			}
			node = ParseExpr(cmp, 0);
			Accept(cmp, ')');
			break;
		case TK_PP:
			node = ParseUnary(cmp, PREFIX_PP);
			AnalyseIncDec(cmp, node);
			break;
		case TK_MM:
			node = ParseUnary(cmp, PREFIX_MM);
			AnalyseIncDec(cmp, node);
			break;
		default:
			Error(cmp, 0, "invalid expression\n");
			Lex(cmp);
			break;
	}

	return ParsePostfix(cmp, node);
}

static node_t *ParsePostfix(compiler_t *cmp, node_t *base)
{
	if (!cmp || !base)
		return NULL;

	while (true) {
		switch (cmp->token) {
			case TK_PP:
				base = NewPostfixPPMM(cmp, base, POSTFIX_PP);
				AnalyseIncDec(cmp, base);
				break;
			case TK_MM:
				base = NewPostfixPPMM(cmp, base, POSTFIX_MM);
				AnalyseIncDec(cmp, base);
				break;
			case '[':
				base = ParseArrayAccess(cmp, base);
				AnalyseArrayAccess(cmp, base);
				break;
			case '(':
				base = ParseFunCall(cmp, base);
				AnalyseFunctionCall(cmp, base);
				break;
			case '.':
				base = ParseMemberAccess(cmp, base);
				AnalyseMemberAccess(cmp, base);
				break;
			case TK_ARROW:
				base = ParseMemberAccess(cmp, base);
				base->kind = MEMBER_ARROW_ACCESS;
				AnalyseMemberPtrAccess(cmp, base);
				break;
			default:
				return base;
		}
		Lex(cmp);
	}
}

static node_t *ParseMemberAccess(compiler_t *cmp, node_t *base)
{
	node_t *node;

	if (!cmp || !base)
		return NULL;

	node = NodeNew(MEMBER_ACCESS, cmp->file->line);

	node->memberAccess.base = base;

	Lex(cmp);

	if (!Expect(cmp, TK_ID))
		return node;

	node->memberAccess.member = MiscDuplicate(&cmp->value);

	return node;
}

static node_t *ParseArrayAccess(compiler_t *cmp, node_t *base)
{
	node_t *node;

	if (!cmp || !base)
		return NULL;

	node = NodeNew(ARRAYACCESS, cmp->file->line);

	Lex(cmp);

	node->arrayAccess.base  = base;
	node->arrayAccess.index = ParseExpr(cmp, 0);

	Expect(cmp, ']');

	return node;
}

static node_t *ParseFunCall(compiler_t *cmp, node_t *base)
{
	node_t *node, **ptr;

	if (!cmp || !base)
		return NULL;

	node = NodeNew(FUNCALL, cmp->file->line);

	node->fncall.base = base;

	if (Lex(cmp) == ')')
		return node;

	ptr = &node->fncall.args;

	do {
		*ptr = ParseExpr(cmp, opPrec[',']);

		if (*ptr) {
			(*ptr)->prev_arg = node->prev_arg;
			node->prev_arg = *ptr;
			ptr = &(*ptr)->next_stmt;
		}

		node->fncall.count++;

		if (cmp->token != ',')
			break;

	} while (Lex(cmp));

	Expect(cmp, ')');

	return node;
}

static node_t *NewPostfixPPMM(compiler_t *cmp, node_t *base, nodekind kind)
{
	node_t *node;

	if (!cmp || !base)
		return NULL;

	node = NodeNew(kind, cmp->file->line);

	node->unary.expr = base;

	return node;
}

static node_t *ParseUnary(compiler_t *cmp, nodekind kind)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(kind, cmp->file->line);

	Lex(cmp);

	node->unary.expr = ParsePrefix(cmp);

	return node;
}

static node_t *ParseSizeof(compiler_t *cmp)
{
	node_t *node;
	type_t *type = cmp_primitives[INT];

	if (!cmp)
		return NULL;

	node = NodeNew(LITERAL, cmp->file->line);

	if (Lex(cmp) == '(') {
		Lex(cmp);
		if (IsType(cmp) || IsQualifier(cmp) || IsStorage(cmp))
			type = ParseType(cmp);
		else {
			node_t *tmp = ParseExpr(cmp, 0);
			if (tmp)
				type = tmp->type;
		}
		Accept(cmp, ')');
	}
	else {
		node_t *tmp = ParseExpr(cmp, 0);
		if (tmp)
			type = tmp->type;
	}

	CheckIncompleteType(cmp, type, SCOPE_LOCAL, KW_AUTO);

	node->value    = MiscNew(MISC_INT);
	node->value->i = (int)TypeGetSize(type);

	return node;
}

static node_t *ParseTypecast(compiler_t *cmp)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(TYPECAST, cmp->file->line);

	node->typecast.to = ParseType(cmp);

	Accept(cmp, ')');

	node->typecast.from = ParsePrefix(cmp);

	return node;
}

static node_t *ParseExprNode(compiler_t *cmp, node_t *lhs)
{
	node_t *node;
	int op;

	if (!cmp || !lhs)
		return NULL;

	node = NodeNew(BINARYEXPR, cmp->file->line);

	op = cmp->token;

	Lex(cmp);

	if (op == '=' || (op >= TK_ADDEQU && op <= TK_OREQU)) {
		node->kind = ASSIGN;
		node->bin.lhs = lhs;
		node->bin.op = op;
		node->bin.rhs = ParseExpr(cmp, opPrec[op] - 1);
		AnalyseAssign(cmp, node);
		return node;
	}

	switch (op) {
		case '?':
			node->ternary.cond = lhs;
			return ParseTernary(cmp, node);
		case TK_ANDAND:
			node = NewBinaryExpr(cmp, lhs, SHORT_CIRCUIT_AND, op, opPrec[op]);
			AnalyseShortCircuit(cmp, node);
			break;
		case TK_OROR:
			node = NewBinaryExpr(cmp, lhs, SHORT_CIRCUIT_OR, op, opPrec[op]);
			AnalyseShortCircuit(cmp, node);
			break;
		default:
			node = NewBinaryExpr(cmp, lhs, BINARYEXPR, op, opPrec[op]);
			AnalyseBinaryExpr(cmp, node);
			break;
	}

	return node;
}

static node_t *ParseTernary(compiler_t *cmp, node_t *node)
{
	if (!cmp || !node)
		return NULL;

	node->kind = TERNARY;

	node->ternary._true = ParseExpr(cmp, opPrec['?'] - 1);

	Accept(cmp, ':');

	node->ternary._false = ParseExpr(cmp, opPrec['?'] - 1);

	AnalyseTernary(cmp, node);

	return node;
}

static node_t *NewBinaryExpr(compiler_t *cmp, node_t *lhs, nodekind kind, int op, int prec)
{
	node_t *node;

	if (!cmp)
		return NULL;

	node = NodeNew(kind, cmp->file->line);

	node->bin.lhs = lhs;
	node->bin.rhs = ParseExpr(cmp, prec);
	node->bin.op  = op;

	return node;
}

static node_t *NewLiteral(compiler_t *cmp, type_t *type)
{
	node_t *node;

	if (!cmp || !type)
		return NULL;

	node = NodeNew(LITERAL, cmp->file->line);

	node->value = MiscDuplicate(&cmp->value);
	node->type  = type;

	return node;
}

static node_t *ParseString(compiler_t *cmp)
{
	string_concat_t *sc = NULL, **ptr = &sc;
	char *sPtr = NULL;
	size_t totalLen = 0;
	node_t *node;
	misc_t *string;
	
	if (!cmp)
		return NULL;

	node = NodeNew(STRING, cmp->file->line);

	do {
		*ptr = StrConcatNew(cmp->value.str);
		(*ptr)->len = strlen((*ptr)->str);
		totalLen += (*ptr)->len;
		ptr = &(*ptr)->next;
	} while (Lex(cmp) == TK_STRING);

	node->type = TypeNewArray(cmp_primitives[CHAR], totalLen + 1);

	string = Alloc(sizeof(*string) + totalLen + 1);
	sPtr   = string->str = (char *)(string + 1);

	string->kind = MISC_STR;

	while (sc) {
		memcpy(sPtr, sc->str, sc->len);
		sPtr += sc->len;
		sc = sc->next;
	}

	string->str[totalLen] = '\0';

	node->value = string;

	return node;
}