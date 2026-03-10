#include "compiler.h"

// =======================================

static type_t _void;
static type_t _char;
static type_t _uchar;
static type_t _short;
static type_t _ushort;
static type_t _int;
static type_t _uint;
static type_t _long;
static type_t _ulong;
static type_t _float;
static type_t _double;
static type_t _ldouble;

// =======================================

type_t *cmp_primitives[END_PRIMITIVES] = {
	&_void,  &_char,   &_uchar,
	&_short, &_ushort, &_int,
	&_uint,  &_long,   &_ulong,  
	&_float, &_double, &_ldouble
};

static void Initialize(type_t *ty, const char *name, size_t size, typekind kind);

// =======================================

void InitPrimitives(void)
{
	Initialize(&_void,    "void",			  1,   VOID);
	Initialize(&_char,    "char",	          1,   CHAR);
	Initialize(&_uchar,   "unsigned char",    1,   UCHAR);
	Initialize(&_short,   "short",            2,   SHORT);
	Initialize(&_ushort,  "unsigned short",   2,   USHORT);
	Initialize(&_int,     "int",              4,   INT);
	Initialize(&_uint,    "unsigned int",     4,   UINT);
	Initialize(&_long,    "long",			  4,   LONG);
	Initialize(&_ulong,   "unsigned long",	  4,   ULONG);
	Initialize(&_float,   "float",			  4,   FLOAT);
	Initialize(&_double,  "double",			  8,   DOUBLE);
	Initialize(&_ldouble, "long double",	  16,  LDOUBLE);

	_ldouble.align = 8;
}

static void Initialize(type_t *ty, const char *name, size_t size, typekind kind)
{
	if (!ty || !name)
		return;

	ty->tyname = name;
	ty->kind   = kind;
	ty->size   = size;
	ty->align  = size;
}