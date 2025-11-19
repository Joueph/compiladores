// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include "lexer.h" // Precisa da definição de Token
#include <stdio.h>

// DECLARAÇÃO das variáveis globais que serão usadas no projeto
extern Token token;
extern FILE *inputFile;
extern int enderecoAtual;
extern int errosCompilacao; // Declaração para os outros arquivos verem

#endif //GLOBALS_H