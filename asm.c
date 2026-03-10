#include "compiler.h"

// ==========================================

static node_t			 *ParseASMBlock(compiler_t *cmp);
static asm_instruction_t *ParseInstruction(compiler_t *cmp);
static asm_instruction_t *ParseDataTransfer(compiler_t *cmp);

static asm_operand_t     *ParseOperand(compiler_t *cmp);

// ==========================================

node_t *ParseAsmStmt(compiler_t *cmp)
{
	node_t *node;

	if (!cmp)
		return NULL;

	if (cmp->token == '{')
		return ParseASMBlock(cmp);

	node = NodeNew(ASM_STMT, cmp->file->line);

	node->_asm_stmt.ins = ParseInstruction(cmp);

	return node;
}

static node_t *ParseASMBlock(compiler_t *cmp)
{
	node_t *node, **ptr;

	if (!cmp)
		return NULL;

	node = NodeNew(ASM_BLOCK, cmp->file->line);

	ptr = &node->_asm_block.head;

	while (*cmp->file->src && Lex(cmp) != '}') {
		*ptr = ParseAsmStmt(cmp);
		if (*ptr)
			ptr = &(*ptr)->next_stmt;
	}

	Expect(cmp, '}');

	return node;
}

static asm_instruction_t *ParseInstruction(compiler_t *cmp)
{
	if (!cmp)
		return NULL;

	switch (cmp->token) {
		case ASM_MOV:
			return ParseDataTransfer(cmp);
	}

	return NULL;
}

static asm_instruction_t *ParseDataTransfer(compiler_t *cmp)
{
	// asm_operand_t *src, *dest;

	if (!cmp)
		return NULL;

	return NULL;
}

static asm_operand_t *ParseOperand(compiler_t *cmp)
{
	if (!cmp)
		return NULL;
	return NULL;
}