# Compilador de C

Esse é um projeto pessoal, ainda em desenvolvimento, que visa implementar um compilador de C funcional na linguagem C.

# Demonstração
Atualmente, ao executar o compilador com um arquivo .c, ele printa na tela o codigo intermediario em formato TAC(Three Address Code)
Exemplo:

``` c
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
```

Codigo intermediario gerado:

``` assembly
        ENTER       [main], 16
        ADD         t0, 2, 3
        MUL         t1, t0, 4
        ADD         t2, 1, t1
        ADD         t3, t2, 5
        STORE       [x], t3
        LEA         t4, [item], 4
        COPY        t5, [x]
        STORE       t4, t5
        LEAVE       [main], 16
        ENTER       [Fun], 0
        COPY        t0, [b]
        COPY        t1, [a]
        LSH         t2, t1, 1
        STORE       t0, t2
        LEAVE       [Fun], 0
```

# Progresso

<ol>
  <li>
     Lexer: 80% 
  </li>
  <li>
    Parser: 98%
  </li>
  <li>
    Analisador semantico: 98%
  </li>
  <li>
    Gerador de codigo intermediário: 60%
  </li>
</ol>
