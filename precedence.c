#include "compiler.h"

byte opPrec[ASCII_SIZE];

void InitPrecedence(void)
{
	opPrec[',']       = 1;
	opPrec['=']       = 2;
	opPrec[TK_ADDEQU] = 2;
	opPrec[TK_SUBEQU] = 2;
	opPrec[TK_MULEQU] = 2;
	opPrec[TK_DIVEQU] = 2;
	opPrec[TK_SHLEQU] = 2;
	opPrec[TK_SHREQU] = 2;
	opPrec[TK_XOREQU] = 2;
	opPrec[TK_ANDEQU] = 2;
	opPrec[TK_MODEQU] = 2;
	opPrec['?']       = 3;
	opPrec[TK_OROR]   = 4;
	opPrec[TK_OROR]   = 4;
	opPrec[TK_ANDAND] = 4;
	opPrec['|']       = 5;
	opPrec['^']       = 6;
	opPrec['&']       = 7;
	opPrec[TK_EQUEQU] = 8;
	opPrec[TK_NOTEQU] = 8;
	opPrec['<']       = 8;
	opPrec['>']       = 8;
	opPrec[TK_GE]     = 8;
	opPrec[TK_LE]     = 8;
	opPrec[TK_SHL]	  = 9;
	opPrec[TK_SHR]	  = 9;
	opPrec['+']		  = 10;
	opPrec['-']		  = 10;
	opPrec['*']		  = 11;
	opPrec['/']		  = 11;
	opPrec['%']		  = 11;
}