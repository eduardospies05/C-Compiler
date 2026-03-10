#include "compiler.h"

#define INVALID 0
#define BLANK   1
#define NEWLINE 2
#define LETTER  4
#define DIGIT   8
#define HEX     16
#define OTHER   32
#define STROPEN 64
#define CHROPEN 128

// ==========================================

static void  LexComment(compiler_t *cmp);
static byte  map[ASCII_SIZE];
static void *GetKeyword(compiler_t *cmp);

// ==========================================

int Lex(compiler_t *cmp)
{
	if (!cmp)
		return TK_EOF;

	if (cmp->flags & CMPF_DONT_LEX)
		return cmp->flags &= ~CMPF_DONT_LEX, cmp->token;

	while (true) {
		while (map[*cmp->file->src] & BLANK)
			cmp->file->src++;
		switch (*cmp->file->src++) {
		case 0:
			return cmp->file->src--, cmp->token = TK_EOF;
		case '\n':
			cmp->file->line++;
			continue;
		case ';':
			if (cmp->flags & CMPF_ASSEMBLY_MODE) {
				while (*cmp->file->src && map[*cmp->file->src] ^ NEWLINE)
					cmp->file->src++;
				continue;
			}
		case ':':
		case ',':
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case '?':
			return cmp->token = *(cmp->file->src - 1);
		case '+':
			if (*cmp->file->src == '+') return cmp->file->src++, cmp->token = TK_PP;
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_ADDEQU;
			return cmp->token = '+';
		case '-':
			if (*cmp->file->src == '-') return cmp->file->src++, cmp->token = TK_MM;
			if (*cmp->file->src == '>') return cmp->file->src++, cmp->token = TK_ARROW;
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_SUBEQU;
			return cmp->token = '-';
		case '*':
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_MULEQU;
			return cmp->token = '*';
		case '/':
			if (*cmp->file->src == '/') {
				while (*cmp->file->src && map[*cmp->file->src] ^ NEWLINE)
					cmp->file->src++;
				continue;
			}
			if (*cmp->file->src == '*') { LexComment(cmp); continue; }
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_DIVEQU;
			return cmp->token = '/';
		case '&':
			if (*cmp->file->src == '&') return cmp->file->src++, cmp->token = TK_ANDAND;
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_ANDEQU;
			return cmp->token = '&';
		case '|':
			if (*cmp->file->src == '|') return cmp->file->src++, cmp->token = TK_OROR;
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_OREQU;
			return cmp->token = '|';
		case '^':
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_XOREQU;
			return cmp->token = '^';
		case '=':
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_EQUEQU;
			return cmp->token = '=';
		case '!':
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_NOTEQU;
			return cmp->token = '!';
		case '<':
			if (*cmp->file->src == '<') {
				cmp->file->src++;
				if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_SHLEQU;
				return cmp->token = TK_SHL;
			}
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_LE;
			return cmp->token = '<';
		case '>':
			if (*cmp->file->src == '>') {
				cmp->file->src++;
				if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_SHREQU;
				return cmp->token = TK_SHR;
			}
			if (*cmp->file->src == '=') return cmp->file->src++, cmp->token = TK_GE;
			return cmp->token = '>';
		case '.':
			if (*cmp->file->src == '.' && cmp->file->src[1] == '.') return cmp->file->src += 2, cmp->token = TK_ELIPSIS;
			return cmp->token = '.';
		case '"':
			cmp->value.str = cmp->file->src;
			cmp->value.kind = MISC_STR;
			while (*cmp->file->src && map[*cmp->file->src] ^ STROPEN)
				cmp->file->src++;
			if (!*cmp->file->src) {
				Error(cmp, 0, errorTable[STRING_MISSING_QUOTE]);
				return cmp->token = TK_EOF;
			}
			cmp->value.str = AtomLength(cmp->value.str, cmp->file->src - cmp->value.str);
			return cmp->file->src++, cmp->token = TK_STRING;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			cmp->value.i = 0;
			cmp->file->src--;
			cmp->value.kind = MISC_INT;
			
			do
				cmp->value.i = cmp->value.i * 10 + (*cmp->file->src++ & 0xF);
			while (map[*cmp->file->src] & DIGIT);

			return cmp->token = TK_INT;
			case '_':
			case 'a':
			case 'A':
			case 'b':
			case 'B':
			case 'c':
			case 'C':
			case 'd':
			case 'D':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			case 'h':
			case 'H':
			case 'i':
			case 'I':
			case 'j':
			case 'J':
			case 'k':
			case 'K':
			case 'l':
			case 'L':
			case 'm':
			case 'M':
			case 'n':
			case 'N':
			case 'o':
			case 'O':
			case 'p':
			case 'P':
			case 'q':
			case 'Q':
			case 'r':
			case 'R':
			case 's':
			case 'S':
			case 't':
			case 'T':
			case 'u':
			case 'U':
			case 'v':
			case 'V':
			case 'w':
			case 'W':
			case 'x':
			case 'X':
			case 'y':
			case 'Y':
			case 'z':
			case 'Z':
				cmp->file->src--;
				cmp->value.str = cmp->file->src;
				cmp->value.kind = MISC_STR;

				while (map[*cmp->file->src] & (LETTER | DIGIT))
					cmp->file->src++;

				cmp->value.str = AtomLength(cmp->value.str, cmp->file->src - cmp->value.str);

				cmp->keyword = GetKeyword(cmp);

				if (cmp->keyword)
					return cmp->token;

				return cmp->token = TK_ID;
			default:
				Error(cmp, 0, "invalid token '%c'\n", *(cmp->file->src - 1));
				return cmp->token = TK_ERROR;
		}
	}
}

static void LexComment(compiler_t *cmp)
{
	if (!cmp)
		return;

	do
		if (map[*cmp->file->src++] & NEWLINE)
			cmp->file->line++;
	while (*cmp->file->src && (*cmp->file->src != '*' || cmp->file->src[1] != '/'));

	if (!*cmp->file->src) {
		Error(cmp, 0, "missing '*/'\n");
		return;
	}

	cmp->file->src += 2;
}

static void *GetKeyword(compiler_t *cmp)
{
	if (!cmp)
		return NULL;

	if (cmp->flags & CMPF_ASSEMBLY_MODE)
		return TableIGet(asm_keywords, cmp->value.str);

	return TableGet(keywords, cmp->value.str, true);
}

static byte map[] = {
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ BLANK,
	/*'0'*/ NEWLINE,
	/*'0'*/ INVALID,
	/*'0'*/ BLANK,
	/*'0'*/ BLANK,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*'0'*/ INVALID,
	/*' '*/ BLANK,
	/*'!'*/ OTHER,
	/*'"'*/ STROPEN,
	/*'#'*/ OTHER,
	/*'$'*/ INVALID,
	/*'%'*/ OTHER,
	/*'&'*/ OTHER,
	/*'''*/ CHROPEN,
	/*'('*/ OTHER,
	/*')'*/ OTHER,
	/*'*'*/ OTHER,
	/*'+'*/ OTHER,
	/*','*/ OTHER,
	/*'-'*/ OTHER,
	/*'.'*/ OTHER,
	/*'/'*/ OTHER,
	/*'0'*/ DIGIT | HEX,
	/*'1'*/ DIGIT | HEX,
	/*'2'*/ DIGIT | HEX,
	/*'3'*/ DIGIT | HEX,
	/*'4'*/ DIGIT | HEX,
	/*'5'*/ DIGIT | HEX,
	/*'6'*/ DIGIT | HEX,
	/*'7'*/ DIGIT | HEX,
	/*'8'*/ DIGIT | HEX,
	/*'9'*/ DIGIT | HEX,
	/*':'*/ OTHER,
	/*';'*/ OTHER,
	/*'<'*/ OTHER,
	/*'='*/ OTHER,
	/*'>'*/ OTHER,
	/*'?'*/ OTHER,
	/*'@'*/ OTHER,
	/*'A'*/ LETTER | HEX,
	/*'B'*/ LETTER | HEX,
	/*'C'*/ LETTER | HEX,
	/*'D'*/ LETTER | HEX,
	/*'E'*/ LETTER | HEX,
	/*'F'*/ LETTER | HEX,
	/*'G'*/ LETTER,
	/*'H'*/ LETTER,
	/*'I'*/ LETTER,
	/*'J'*/ LETTER,
	/*'K'*/ LETTER,
	/*'L'*/ LETTER,
	/*'M'*/ LETTER,
	/*'N'*/ LETTER,
	/*'O'*/ LETTER,
	/*'P'*/ LETTER,
	/*'Q'*/ LETTER,
	/*'R'*/ LETTER,
	/*'S'*/ LETTER,
	/*'T'*/ LETTER,
	/*'U'*/ LETTER,
	/*'V'*/ LETTER,
	/*'W'*/ LETTER,
	/*'X'*/ LETTER,
	/*'Y'*/ LETTER,
	/*'Z'*/ LETTER,
	/*'['*/ OTHER,
	/*'\'*/ OTHER,
	/*']'*/ OTHER,
	/*'^'*/ OTHER,
	/*'_'*/ LETTER,
	/*'`'*/ OTHER,
	/*'a'*/ LETTER | HEX,
	/*'b'*/ LETTER | HEX,
	/*'c'*/ LETTER | HEX,
	/*'d'*/ LETTER | HEX,
	/*'e'*/ LETTER | HEX,
	/*'f'*/ LETTER | HEX,
	/*'g'*/ LETTER,
	/*'h'*/ LETTER,
	/*'i'*/ LETTER,
	/*'j'*/ LETTER,
	/*'k'*/ LETTER,
	/*'l'*/ LETTER,
	/*'m'*/ LETTER,
	/*'n'*/ LETTER,
	/*'o'*/ LETTER,
	/*'p'*/ LETTER,
	/*'q'*/ LETTER,
	/*'r'*/ LETTER,
	/*'s'*/ LETTER,
	/*'t'*/ LETTER,
	/*'u'*/ LETTER,
	/*'v'*/ LETTER,
	/*'w'*/ LETTER,
	/*'x'*/ LETTER,
	/*'y'*/ LETTER,
	/*'z'*/ LETTER,
	/*'{'*/ OTHER,
	/*'|'*/ OTHER,
	/*'}'*/ OTHER,
	/*'~'*/ OTHER,
};