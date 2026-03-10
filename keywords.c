#include "compiler.h"

#define NEWKEYWORD(name, token) TableAdd(keywords, Atom(name), (void *)token)
#define NEWASMKEY(name, token) TableAdd(asm_keywords, Atom(name), (void *)token)

hashtable_t *keywords;
hashtable_t *asm_keywords;

void InitKeywords(void)
{
	if (!keywords)
		keywords = TableNew(true), asm_keywords = TableNew(true);

	NEWKEYWORD("while",    KW_WHILE);
	NEWKEYWORD("break",    KW_BREAK);
	NEWKEYWORD("return",   KW_RETURN);
	NEWKEYWORD("do",       KW_DO);
	NEWKEYWORD("if",       KW_IF);
	NEWKEYWORD("else",     KW_ELSE);
	NEWKEYWORD("for",      KW_FOR);
	NEWKEYWORD("goto",     KW_GOTO);
	NEWKEYWORD("continue", KW_CONTINUE);
	NEWKEYWORD("switch",   KW_SWITCH);
	NEWKEYWORD("case",     KW_CASE);
	NEWKEYWORD("default",  KW_DEFAULT);
	NEWKEYWORD("typedef",  KW_TYPEDEF);
	NEWKEYWORD("static",   KW_STATIC);
	NEWKEYWORD("extern",   KW_EXTERN);
	NEWKEYWORD("auto",     KW_AUTO);
	NEWKEYWORD("register", KW_REGISTER);
	NEWKEYWORD("const",    KW_CONST);
	NEWKEYWORD("volatile", KW_VOLATILE);
	NEWKEYWORD("void",     KW_VOID);
	NEWKEYWORD("signed",   KW_SIGNED);
	NEWKEYWORD("unsigned", KW_UNSIGNED);
	NEWKEYWORD("char",     KW_CHAR);
	NEWKEYWORD("short",    KW_SHORT);
	NEWKEYWORD("int",      KW_INT);
	NEWKEYWORD("long",     KW_LONG);
	NEWKEYWORD("float",    KW_FLOAT);
	NEWKEYWORD("double",   KW_DOUBLE);
	NEWKEYWORD("struct",   KW_STRUCT);
	NEWKEYWORD("union",    KW_UNION);
	NEWKEYWORD("enum",     KW_ENUM);
	NEWKEYWORD("sizeof",   KW_SIZEOF);
	NEWKEYWORD("__asm",    KW__ASM);

// ============= X86 ASSEMBLY KEYWORDS ================

	NEWASMKEY("EAX", ASM_EAX);
	NEWASMKEY("EBX", ASM_EBX);
	NEWASMKEY("ECX", ASM_ECX);
	NEWASMKEY("EDX", ASM_EDX);
	NEWASMKEY("ESI", ASM_ESI);
	NEWASMKEY("EDI", ASM_EDI);
	NEWASMKEY("EBP", ASM_EBP);
	NEWASMKEY("ESP", ASM_ESP);

	NEWASMKEY("AX", ASM_AX);
	NEWASMKEY("BX", ASM_BX);
	NEWASMKEY("CX", ASM_CX);
	NEWASMKEY("DX", ASM_DX);
	NEWASMKEY("SI", ASM_SI);
	NEWASMKEY("DI", ASM_DI);
	NEWASMKEY("BP", ASM_BP);
	NEWASMKEY("SP", ASM_SP);

	NEWASMKEY("AL", ASM_AL);
	NEWASMKEY("AH", ASM_AH);
	NEWASMKEY("BL", ASM_BL);
	NEWASMKEY("BH", ASM_BH);
	NEWASMKEY("CL", ASM_CL);
	NEWASMKEY("CH", ASM_CH);
	NEWASMKEY("DL", ASM_DL);
	NEWASMKEY("DH", ASM_DH);

	NEWASMKEY("MOV", ASM_MOV);
}