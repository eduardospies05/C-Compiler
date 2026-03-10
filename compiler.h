#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "tokens.h"
#include "error.h"
#include <stdarg.h>

#define ARENA_MISC   0
#define ARENA_AST    1
#define ARENA_SOURCE 2
#define ARENA_FIXED  3
#define ARENA_IR     4
#define ARENA_CODE   5
#define ARENA_MAX    6

#define TABLE_SYMBOLS  0
#define TABLE_TYPEDEFS 1
#define TABLE_STRUCTS  2
#define TABLE_UNIONS   3
#define TABLE_ENUMS    4
#define TABLE_LABELS   5
#define TABLE_MAX      6

#define SCOPE_GLOBAL 0
#define SCOPE_PARAM  1
#define SCOPE_LOCAL  2

#define ASCII_SIZE 0x7F

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t dword;

// ==========================================

#include "misckind.h"

typedef struct misc_s
{
	misckind kind;
	union {
		char   c;
		long   i;
		double d;
		struct symbol_s    *sym;
		struct label_s     *lb;
		struct field_s     *field;
		struct temporary_s *reg;
		char *str;
	};
}misc_t;

misc_t *MiscNew(misckind kind);
misc_t *MiscDuplicate(misc_t *obj);
misc_t *MiscNewLabel(const char *opt_name, size_t opt_id);
misc_t *MiscNewTempReg(size_t id);
misc_t *MiscNewInt(int value);

// ==========================================

#include "instructionkind.h"

typedef struct instruction_s
{
	instructionkind kind;
	misc_t  *dest;
	misc_t  *arg1;
	misc_t  *arg2;
	misc_t ***jmpTable;
	struct  type_s *type;
	size_t  line;
	struct  instruction_s *prev, *next;
}instruction_t;

instruction_t *InstructionNew(instructionkind kind, misc_t *dest, misc_t *arg1, misc_t *arg2, struct type_s *type, size_t line);

// ==========================================

typedef struct temporary_s
{
	size_t id;
	int    real_reg;
}temporary_t;

// ==========================================

typedef struct label_s
{
	const char *name;
	size_t      id;
	size_t      addr;
}label_t;

// ==========================================

typedef struct patcher_s
{
	const char *name;
	misc_t	  **data;
	size_t      line;
	struct patcher_s *next;
}patcher_t;

patcher_t *PatcherNew(const char *name, misc_t **data, size_t line);

// ==========================================

typedef struct symbol_s
{
	const char    *name;
	struct type_s *type;
	misc_t        *value; // enum
	size_t         scope;
	size_t         storage;
	size_t         references;
	size_t         line;
	union {
		bool           initialized;
		bool		   isPrototype;
	};
}symbol_t;

symbol_t *SymbolNew(const char *name, struct type_s *type, size_t scope, size_t storage, size_t line);

// ==========================================

typedef struct parameter_s
{
	symbol_t           *sym;
	struct type_s      *type;
	struct parameter_s *next;
}parameter_t;

parameter_t *ParameterNew(symbol_t *sym, struct type_s *type);

// ==========================================

typedef struct field_s
{
	const   char   *name;
	struct  type_s *type;
	size_t  offset;
	size_t  bitfield;
	struct  field_s *next;
}field_t;

field_t *FieldNew(const char *name, struct type_s *type);

// ==========================================

#include "typekind.h"

typedef struct type_s
{
	const char    *tyname;
	typekind       kind;
	struct type_s *base;
	size_t         size;
	size_t         align;
	union {
		struct {
			struct hashtable_s *members;
			field_t            *fields;
			bool				hasDefinition;
		};
		struct {
			parameter_t *params;
			bool         hasVoidParam;
			bool         hasElipsis;
			size_t       paramCount;
		};
	};
}type_t;

extern type_t *cmp_primitives[];

type_t* TypeNew(typekind kind);
type_t* TypeNewFunction(type_t* ret, size_t paramCount, bool hasVoidParam, bool hasElipsis, parameter_t* params);
type_t *TypeNewPtr(type_t *base);
type_t *TypeNewArray(type_t *base, size_t length);
size_t  TypeGetSize(type_t *type);
size_t  TypeGetAlign(type_t *type);
type_t *TypeNewDerived(typekind kind, const char *name);

void    InitPrimitives(void);

// ==========================================

typedef struct file_s
{
	const char         *path;
	char               *src;
	size_t              line;
	size_t              errors;
	size_t              warnings;
	struct hashtable_s *statics;
}file_t;

file_t *FileNew(const char *path);

// ==========================================

typedef struct hashtable_s hashtable_t;

hashtable_t *TableNew(bool fixed);
void         TableAdd(hashtable_t *table, const char *key, void *data);
void        *TableGet(hashtable_t *table, const char *key, bool localSearch);
void         TablePush(hashtable_t **table);
void         TablePop(hashtable_t **table);
size_t       TableGetScope(hashtable_t *table);
void 		*TableGetDefault(hashtable_t *table);
void		 TableSetDefault(hashtable_t *table, void *ptr);
void        *TableIGet(hashtable_t *table, const char *key); // insensitive
void	   **TableToArray(hashtable_t *table);
size_t       TableGetSize(hashtable_t *table);
bool		 TableGetJumpTable(hashtable_t *table);
void		 TableSetJumpTable(hashtable_t *table);

// ==========================================

typedef struct string_concat_s
{
	const  char *str;
	size_t       len;
	struct string_concat_s *next;
}string_concat_t;

string_concat_t *StrConcatNew(const char *str);

// ==========================================

#include "asmkind.h"

typedef struct asm_operand_s {
	asmkind kind;
	int     segment;
	byte    size;
	union {
		struct {
			int base;
			int index;
			int scale;
			int disp;
		}mem;
		int       reg;
		int       imm;
		symbol_t *sym;
	};
}asm_operand_t;

typedef struct asm_instruction_s
{
	int            opcode;
	asm_operand_t *dest;
	asm_operand_t *src;
	asm_operand_t *ex;
	size_t         line;
	byte           size;
}asm_instruction_t;

asm_operand_t     *ASM_OperandNew(asmkind kind);
asm_instruction_t *ASM_InstructionNew(int opcode, asm_operand_t *dest, asm_operand_t *src, asm_operand_t *ex, byte size, size_t line);

// ==========================================

#define CMPF_ERROR            (1 << 0)
#define CMPF_DISABLE_WARNINGS (1 << 1)
#define CMPF_DONT_LEX		  (1 << 2)
#define CMPF_DONT_ENTER_SCOPE (1 << 3)
#define CMPF_LOCAL_SCOPE	  (1 << 4)
#define CMPF_IGNORE_ID		  (1 << 5)
#define CMPF_MAYBE_LABEL      (1 << 6)
#define CMPF_ASSEMBLY_MODE    (1 << 7)
#define CMPF_MEMORY_OPERATION (1 << 8)

typedef struct node_s node_t;

typedef struct compiler_s
{
	hashtable_t   *table[TABLE_MAX];
	file_t        *file;
	node_t        *nodes;
	type_t        *currentFnType;
	patcher_t    **patcher;
	instruction_t *head;
	instruction_t *tail;
	misc_t         value;
	size_t         lpCount;
	int            flags;
	union {
		int      token;
		void    *keyword;
		size_t   stackSubSize;
	};
}compiler_t;

extern byte opPrec[];
extern const char *errorTable[];
extern hashtable_t *keywords;
extern hashtable_t *asm_keywords;

void InitPrecedence(void);
void InitKeywords(void);
void Boot(int argc, char **argv);
int  Lex(compiler_t *cmp);

bool IsType(compiler_t *cmp);
bool IsQualifier(compiler_t *cmp);
bool IsStorage(compiler_t *cmp);

type_t *ParseDeclaration_Level0(compiler_t *cmp, size_t *sclass);
type_t *ParseDeclaration_Level1(compiler_t *cmp, type_t *base, const char **name);
type_t *ParseDeclaration_Level2(compiler_t *cmp, type_t *base);

node_t *ParseTranslationUnit(compiler_t *cmp);
node_t *ParseDeclaration(compiler_t *cmp);
node_t *ParseExpr(compiler_t *cmp, int power);
node_t *ParseStmt(compiler_t *cmp, hashtable_t *cases);
node_t *ParseAsmStmt(compiler_t *cmp);

type_t *ParseUnionStruct(compiler_t *cmp, int table);
type_t *ParseEnum(compiler_t *cmp);

void    GenIR(compiler_t *cmp);

void AddIR(compiler_t *cmp, instruction_t *ins);
void Error(compiler_t *cmp, size_t opt_line, const char *msg, ...);
void Warning(compiler_t *cmp, size_t opt_line, const char *msg, ...);
bool Expect(compiler_t *cmp, int tokenex);
void Accept(compiler_t *cmp, int tokenex);

void      CompilerAddSymbol(compiler_t *cmp, symbol_t *sym);
symbol_t *CompilerGetSymbol(compiler_t *cmp, const char *name, bool localSearch);
void      CompilerNewScope(compiler_t *cmp);
void	  CompilerDelScope(compiler_t *cmp);

instructionkind OpToKind(int op, bool memoryref);
void			PrintInstructions(compiler_t *cmp);

void    CheckIncompleteType(compiler_t *cmp, type_t *type, size_t scope, size_t storage);
type_t *DropQualifiers(type_t *type);
bool    IsEqual(type_t *t1, type_t *t2);
bool    IsCompatible(type_t *t1, type_t *t2);
bool    IsPrimitive(type_t *type);
bool    IsPointer(type_t *type);
bool    IsArith(type_t *type);
bool    IsLeftValue(node_t *node);
bool    IsFunction(type_t *t1);
bool	IsInteger(type_t *type);
bool    IsComposite(type_t *type);
bool    IsStruct(type_t *type);
bool    IsUnion(type_t *type);
bool    IsEnum(type_t *type);

type_t *GetBaseType(type_t *type);
type_t *ParseType(compiler_t *cmp);

int     SolveExpr(compiler_t *cmp, node_t *node);

// ==========================================

void  AnalyseLiteral(compiler_t *cmp, node_t *node);
void  AnalyseBinaryExpr(compiler_t *cmp, node_t *node);
void  AnalyseAssign(compiler_t *cmp, node_t *node);
void  AnalyseFunction(compiler_t *cmp, node_t *node);
void  AnalyseVarDecl(compiler_t *cmp, node_t *node);
void  AnalyseIdentifier(compiler_t *cmp, node_t *node);
void  AnalyseCondition(compiler_t *cmp, node_t *node);
void  AnalyseWhile(compiler_t *cmp, node_t *node);
void  AnalyseReturn(compiler_t *cmp, node_t *node);
void  AnalyseAddressOf(compiler_t *cmp, node_t *node);
void  AnalyseDereference(compiler_t *cmp, node_t *node);
void  AnalyseTypecast(compiler_t *cmp, node_t *node);
void  AnalyseIncDec(compiler_t *cmp, node_t *node);
void  AnalyseArrayAccess(compiler_t *cmp, node_t *node);
void  AnalyseUnaryPlusMinus(compiler_t *cmp, node_t *node);
void  AnalyseFunctionCall(compiler_t *cmp, node_t *node);
void  AnalyseMemberAccess(compiler_t *cmp, node_t *node);
void  AnalyseMemberPtrAccess(compiler_t *cmp, node_t *node);
void  AnalyseTernary(compiler_t *cmp, node_t *node);
void  AnalyseShortCircuit(compiler_t *cmp, node_t *node);
void  AnalyseNegation(compiler_t *cmp, node_t *node);
void  AnalyseStrInit(compiler_t *cmp, node_t *node, type_t *type);

// ==========================================

#include "nodekind.h"

struct node_s
{
	nodekind kind;
	type_t  *type;
	size_t   line;
	union {
		misc_t *value;
		struct {
			node_t *lhs;
			node_t *rhs;
			int     op;
		}bin;
		struct {
			node_t *cond;
			node_t *then;
			node_t *_else;
		}_if;
		struct {
			node_t *cond;
			node_t *then;
		}_while;
		struct {
			node_t *init;
			node_t *cond;
			node_t *step;
			node_t *then;
		}_for;
		struct {
			node_t   *init;
			node_t   *next;
			symbol_t *sym;
		}decl;
		struct {
			node_t *head;
		}block;
		struct {
			node_t *cond;
			long    valCond;
			node_t *statement;
			hashtable_t *parentSwitch;
			misc_t *label;
		}_case;
		struct {
			hashtable_t *cases_table;
			node_t *cond;
			node_t *body;
		}_switch;
		struct {
			node_t *expr;
		}unary;
		struct {
			misc_t *lb;
			node_t *stmt;
		}label;
		struct {
			size_t  count;
			node_t *head;
		}arrayInit;
		struct {
			size_t pos_num;
			node_t *value;
		}arrayPosInit;
		struct {
			asm_instruction_t *ins;
		}_asm_stmt;
		struct {
			node_t *head;
		}_asm_block;
		struct {
			type_t *to;
			node_t *from;
		}typecast;
		struct {
			node_t *base;
			node_t *index;
		}arrayAccess;
		struct {
			node_t *base;
			node_t *args;
			size_t  count;
		}fncall;
		struct {
			node_t *base;
			misc_t *member;
		}memberAccess;
		struct {
			node_t *cond;
			node_t *_true;
			node_t *_false;
		}ternary;
	};
	node_t *next_stmt;
	node_t *prev_arg;
};

node_t *NodeNew(nodekind kind, size_t line);

// ==========================================

void  *Alloc(size_t nbytes);
void  *IAlloc(size_t nbytes, size_t index);
void   ResetArena(size_t index);
void   DestroyAll(void);

char  *Atom(const char *str);
char  *AtomLength(const char *str, size_t length);
char  *AtomI32(long num);
bool   IStrCmp(const char *s1, const char *s2);

void   CompilerShutdown(const char *error);
size_t Align(size_t a, size_t b);

// ==========================================