#pragma once

typedef enum typekind
{
	VOID,
	CHAR,
	UCHAR,
	SHORT,
	USHORT,
	INT,
	UINT,
	LONG,
	ULONG,
	FLOAT,
	DOUBLE,
	LDOUBLE,
	END_PRIMITIVES,
	ARRAY,
	PTR,
	FUNCTION,
	STRUCT,
	UNION,
	ENUM,
	CONST,
	VOLATILE
}typekind;