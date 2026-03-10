typedef int bool;
typedef unsigned long size_t;

typedef struct item_s
{
	int x;
	int y;
	struct item_s *next;
}item_t;

int main(void)
{
	int x = 1 + (2 + 3) * 4 + 5;
	item_t item;

	item.y = x;

	return 0;
}

static void Fun(int a, int *b)
{
	*b = a << 1;
}

// by Eduardo Spies Acauan