// globals.c
#include "lexer.h" // Precisa da definição de Token
#include <stdio.h>

// DEFINIÇÃO REAL das variáveis globais
Token token;
FILE *inputFile = NULL;
int enderecoAtual = 0; // Variáveis globais começam no endereço 0