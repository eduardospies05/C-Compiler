#pragma once

typedef enum instructionkind {
	IR_COPY,       // dest = src1 (mov)

	IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
	IR_AND, IR_OR,  IR_XOR, IR_SHL, IR_SHR,

	// Unary
	IR_NEG,        // dest = -src1
	IR_NOT,        // dest = ~src1 (bitwise)

	IR_EQU, IR_NOTEQU, IR_LT, IR_GT, IR_LE, IR_GE,

	// Control Flow
	IR_LABEL,      // name:
	IR_JMP,        // jump to label
	IR_JZ,         // jump if src1 == 0
	IR_JNZ,        // jump if src1 != 0

	IR_ARG,        // push src1 (as argument)
	IR_CALL,       // dest = call src1
	IR_RET,        // return src1

	IR_LOAD,       // dest = *src1
	IR_STORE,      // *dest = src1
	IR_LEA,        // dest = &src1 (Address of)

	IR_ENTER,
	IR_LEAVE,
	MAX_INS
} instructionkind;