#include "compiler.h"

// ==========================================

const char *instruction[] = {
    "COPY",     "ADD",      "SUB",      "MUL",
    "DIV",      "MOD",      "AND",      "OR",
    "XOR",      "LSH",      "RSH",      "NEG",
    "NOT",      "EQ",       "NE",       "LT",
    "GT",       "LE",       "GE",       "LABEL",
    "JMP",      "JZ",       "JNZ",      "ARG",
    "CALL",     "RET",      "LOAD",     "STORE",
    "LEA",      "ENTER",    "LEAVE"
};

static void PrintIR(instruction_t *ins);
static void PrintMisc(misc_t *misc);

// ==========================================

void PrintInstructions(compiler_t *cmp)
{
	static unsigned pass_count = 0;
	
	if (!cmp)
		return;

	printf("PASS %02X\n", pass_count++);

	for (instruction_t *i = cmp->head; i; i = i->next)
		PrintIR(i);
}

static void PrintIR(instruction_t *ins)
{
	if (!ins)
		return;

	if (ins->kind == IR_LABEL) {
		if (ins->dest->lb->name) {
			printf("%s:\n", ins->dest->lb->name);
			return;
		}
		printf("@L%i:\n", ins->dest->lb->id);
		return;
	}

	printf("\t%-12s", instruction[ins->kind]);

	if (ins->dest)
		PrintMisc(ins->dest);

	if (ins->arg1) {
		printf(", ");
		PrintMisc(ins->arg1);
	}

	if (ins->arg2) {
		printf(", ");
		PrintMisc(ins->arg2);
	}

	printf("\n");
}

static void PrintMisc(misc_t *misc)
{
	if (!misc)
		return;

	switch (misc->kind) {
		case MISC_SYM:
			printf("[%s]", misc->sym->name);
			return;
		case MISC_INT:
			printf("%i", misc->i);
			return;
		case MISC_TEMP:
			printf("t%i", misc->reg->id);
			return;
		case MISC_STR:
			printf("\"%s\"", misc->str);
			return;
		case MISC_LABEL:
			if (!misc->lb->name) {
				printf("@L%i", misc->lb->id);
				return;
			}
			printf("%s", misc->lb->name);
			return;
	}
}