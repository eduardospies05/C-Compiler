#pragma once

#define KEYWORD  0x100
#define ASM_KEYWORD 0x200

#define TK_ERROR		-1
#define TK_EOF			 0
#define TK_ID			 1
#define TK_INT			 2
#define TK_LONG          3
#define TK_UNSIGNED      4
#define TK_UNSIGNED_LONG 5
#define TK_PP			 6
#define TK_MM			 7
#define TK_ARROW		 8
#define TK_ANDAND        9
#define TK_OROR          10
#define TK_ADDEQU        11
#define TK_SUBEQU        12
#define TK_MULEQU        13
#define TK_DIVEQU        14
#define TK_XOREQU        15
#define TK_ANDEQU        16
#define TK_SHLEQU        17
#define TK_SHREQU        18
#define TK_MODEQU		 19
#define TK_OREQU		 20
#define TK_SHL			 21
#define TK_SHR			 22
#define TK_EQUEQU		 23
#define TK_NOTEQU		 24
#define TK_STRING		 25
#define TK_GE			 26
#define TK_LE			 27
#define TK_ELIPSIS		 28
#define TK_NEWLINE		 29
#define TK_FLOAT         30
#define TK_DOUBLE        31

#define KW_WHILE    (KEYWORD)
#define KW_BREAK    (KEYWORD + 1)
#define KW_RETURN   (KEYWORD + 2)
#define KW_FOR      (KEYWORD + 3)
#define KW_DO       (KEYWORD + 4)
#define KW_IF       (KEYWORD + 5)
#define KW_ELSE     (KEYWORD + 6)
#define KW_GOTO     (KEYWORD + 7)
#define KW_CONTINUE (KEYWORD + 8)
#define KW_SWITCH   (KEYWORD + 9)
#define KW_CASE     (KEYWORD + 10)
#define KW_DEFAULT  (KEYWORD + 11)
#define KW_TYPEDEF  (KEYWORD + 12)
#define KW_STATIC   (KEYWORD + 13)
#define KW_EXTERN   (KEYWORD + 14)
#define KW_AUTO     (KEYWORD + 15)
#define KW_REGISTER (KEYWORD + 16)
#define KW_CONST    (KEYWORD + 17)
#define KW_VOLATILE (KEYWORD + 18)
#define KW_VOID     (KEYWORD + 19)
#define KW_SIGNED   (KEYWORD + 20)
#define KW_UNSIGNED (KEYWORD + 21)
#define KW_CHAR     (KEYWORD + 22)
#define KW_SHORT    (KEYWORD + 23)
#define KW_INT      (KEYWORD + 24)
#define KW_LONG     (KEYWORD + 25)
#define KW_FLOAT    (KEYWORD + 26)
#define KW_DOUBLE   (KEYWORD + 27)
#define KW_STRUCT   (KEYWORD + 28)
#define KW_UNION    (KEYWORD + 29)
#define KW_ENUM     (KEYWORD + 30)
#define KW_SIZEOF   (KEYWORD + 31)
#define KW__ASM     (KEYWORD + 32)

#define MASK_VOID     (1 << 2)
#define MASK_CHAR     (1 << 6)
#define MASK_SIGNED   (1 << 7)
#define MASK_UNSIGNED (1 << 10)
#define MASK_SHORT    (1 << 13)
#define MASK_INT      (1 << 15)
#define MASK_LONG     (1 << 17)
#define MASK_FLOAT    (1 << 19)
#define MASK_DOUBLE   (1 << 21)
#define MASK_STRUCT   (1 << 23)
#define MASK_UNION    (1 << 25)
#define MASK_ENUM     (1 << 27)
#define MASK_ID       (1 << 30)

#define ASM_MOV		(ASM_KEYWORD + 0)
#define ASM_INT		(ASM_KEYWORD + 1)
#define ASM_ADD		(ASM_KEYWORD + 2)
#define ASM_PUSH	(ASM_KEYWORD + 3)
#define ASM_EAX 	(ASM_KEYWORD + 4)
#define ASM_EBX 	(ASM_KEYWORD + 5)
#define ASM_ECX 	(ASM_KEYWORD + 6)
#define ASM_EDX 	(ASM_KEYWORD + 7)
#define ASM_EBP 	(ASM_KEYWORD + 8)
#define ASM_ESP 	(ASM_KEYWORD + 9)
#define ASM_ESI 	(ASM_KEYWORD + 10)
#define ASM_EDI 	(ASM_KEYWORD + 11)
#define ASM_DB  	(ASM_KEYWORD + 12)
#define ASM_DW  	(ASM_KEYWORD + 13)
#define ASM_DD  	(ASM_KEYWORD + 14)
#define ASM_AX		(ASM_KEYWORD + 15)
#define ASM_BX		(ASM_KEYWORD + 16)
#define ASM_CX		(ASM_KEYWORD + 17)
#define ASM_DX		(ASM_KEYWORD + 18)
#define ASM_AH		(ASM_KEYWORD + 19)
#define ASM_AL		(ASM_KEYWORD + 20)
#define ASM_BH		(ASM_KEYWORD + 21)
#define ASM_BL		(ASM_KEYWORD + 22)
#define ASM_CH		(ASM_KEYWORD + 23)
#define ASM_CL		(ASM_KEYWORD + 24)
#define ASM_DH		(ASM_KEYWORD + 25)
#define ASM_DL		(ASM_KEYWORD + 26)
#define ASM_BP		(ASM_KEYWORD + 27)
#define ASM_SP		(ASM_KEYWORD + 28)
#define ASM_SI		(ASM_KEYWORD + 29)
#define ASM_DI		(ASM_KEYWORD + 30)