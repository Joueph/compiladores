// lexer.h
#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

typedef enum {
    // ... (símbolos como antes) ...
    SIMBOLO_ERRO, SPROGRAMA, SIDENTIFICADOR, SPONTOVIRGULA, SPONTO,
    SINICIO, SFIM, SVAR, SINTEIRO, SBOOLEANO, SPROCEDIMENTO, SFUNCAO,
    SDOISPONTOS, SATRIBUICAO, SSE, SQUANTO, SCOMANDO, SENQUANTO, SFACA,
    SESCREVA, SLEIA, SABREPARENTESES, SFECHAPARENTESES, SVIRGULA,
    SVERDADEIRO, SFALSO, SOU, SE, SNAO,
    SMAIS, SMENOS, SMULT, SDIV,
    SIGUAL, SMENOR, SMAIOR, SDIFERENTE, SMENORIGUAL, SMAIORIGUAL,
    ENTAO, SSENAO, SNUMERO
} Simbolo;

typedef struct {
    char lexema[50];
    Simbolo simbolo;
} Token;

// Função principal do analisador léxico
void getToken();


// ADICIONADO: Declaração da função de erro léxico
void erro_lexico(const char* mensagem, const char* lexema);

#endif //LEXER_H