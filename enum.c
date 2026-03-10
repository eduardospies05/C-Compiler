#include "compiler.h"

type_t *ParseEnum(compiler_t *cmp)
{
	type_t *ty;
	int     counter = 0;

	if (!cmp)
		return cmp_primitives[INT];

	if (cmp->token == TK_ID) {
		const char *name = cmp->value.str;
		if (!(ty = TableGet(cmp->table[TABLE_ENUMS], name, true)))
			ty = TypeNewDerived(ENUM, name), TableAdd(cmp->table[TABLE_ENUMS], name, ty);
		Lex(cmp);
	}
	else
		ty = TypeNewDerived(ENUM, NULL);

	ty->size  = sizeof(int);
	ty->align = sizeof(int);

	if (cmp->token != '{')
		return ty;

	if (Lex(cmp) == '}') {
		Error(cmp, 0, "enum body expected\n");
		Lex(cmp);
		return ty;
	}

	do {
		const char *vname;
		misc_t     *value;
		symbol_t   *sym;

		if (!Expect(cmp, TK_ID))
			continue;

		vname = cmp->value.str;

		Lex(cmp);

		value = Alloc(sizeof(*value));

		value->kind = MISC_INT;

		if (cmp->token != '=')
			value->i = counter++;
		else {
			node_t *expr;

			Accept(cmp, '=');

			expr = ParseExpr(cmp, opPrec[',']);

			if (expr)
				value->i = SolveExpr(cmp, expr);
		}

		sym = SymbolNew(vname, cmp_primitives[LONG], TableGetScope(cmp->table[TABLE_SYMBOLS]), KW_AUTO, cmp->file->line);

		sym->value = value;

		CompilerAddSymbol(cmp, sym);

		if (cmp->token != ',')
			break;

		if (Lex(cmp) == '}')
			break;

	} while (*cmp->file->src);

	Accept(cmp, '}');

	return ty;
}