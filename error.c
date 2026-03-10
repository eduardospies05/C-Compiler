#include "compiler.h"

const char *errorTable[MAX_ERRORS] = {
	"expression must have arithmetic or pointer type\n",
	"pointer expected\n",
	"valid lvalue expected\n",
	"expression must have integral type\n",
	"expression must be a modifiable value\n",
	"unnamed prototyped parameters not allowed when body is present\n",
	"cannot use break statement outside of a loop and switch case\n",
	"cannot use continue statement outside of a loop\n",
	"a declaration cannot have an label\n",
	"cannot divide by zero\n",
	"cannot use case label outside of a switch statement\n",
	"cannot use default label outside of a switch statement\n",
	"default label already declared\n",
	"a array cannot have negative length\n",
	"incomplete type not allowed\n",
	"cannot initialize a array with zero elements\n",
	"too many initializer values\n",
	"expression must have arithmetic type\n",
	"function expected\n",
	"too many arguments\n",
	"too few arguments\n",
	"expression must have struct or union type\n",
	"missing string end quote\n"
};