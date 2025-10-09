#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

// Enumeração de todos os símbolos (tokens) da linguagem LPD
typedef enum {
    SIMBOLO_ERRO, SPROGRAMA, SIDENTIFICADOR, SPONTOVIRGULA, SPONTO,
    SINICIO, SFIM, SVAR, SINTEIRO, SBOOLEANO, SPROCEDIMENTO, SFUNCAO,
    SDOISPONTOS, SATRIBUICAO, SSE, SQUANTO, SCOMANDO, SENQUANTO, SFACA,
    SESCREVA, SLEIA, SABREPARENTESES, SFECHAPARENTESES, SVIRGULA,
    SVERDADEIRO, SFALSO, SOU, SE, SNAO,
    SMAIS, SMENOS, SMULT, SDIV,
    SIGUAL, SMENOR, SMAIOR, SDIFERENTE, SMENORIGUAL, SMAIORIGUAL,
    SENTAO, SSENAO, SNUMERO
} Simbolo;

// Estrutura para representar um Token
typedef struct {
    char lexema[50];
    Simbolo simbolo;
    int linha;
} Token;

// Função principal do analisador léxico
void getToken(FILE *file);

#endif //LEXER_H