// lexer.c
#include "lexer.h"
#include "globals.h" // Importa 'token' e 'inputFile'
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Tabela de palavras reservadas
const char* palavrasReservadas[] = {
    "programa", "inicio", "fim", "var", "inteiro", "booleano",
    "procedimento", "funcao", "se", "enquanto", "faca", "escreva", "leia",
    "verdadeiro", "falso", "ou", "e", "nao", "entao", "senao", "div"
};

Simbolo simbolosReservados[] = {
    SPROGRAMA, SINICIO, SFIM, SVAR, SINTEIRO, SBOOLEANO,
    SPROCEDIMENTO, SFUNCAO, SSE, SENQUANTO, SFACA, SESCREVA, SLEIA,
    SVERDADEIRO, SFALSO, SOU, SE, SNAO, ENTAO, SSENAO, SDIV
};


void erro_lexico(const char* mensagem, const char* lexema) {
    printf("Erro Lexico: %s ['%s']\n", mensagem, lexema);
    exit(1);
}

int isPalavraReservada(const char* lexema) {
    for (int i = 0; i < sizeof(simbolosReservados) / sizeof(Simbolo); i++) {
        if (strcmp(lexema, palavrasReservadas[i]) == 0) {
            token.simbolo = simbolosReservados[i];
            return 1;
        }
    }
    return 0;
}

void getToken() {
    int i = 0;
    char c;
    int state = 0;
    memset(token.lexema, 0, sizeof(token.lexema));

    while ((c = fgetc(inputFile)) != EOF) {
        switch (state) {
            case 0: // Estado inicial
                if (isspace(c)) continue;
                if (isalpha(c)) {
                    token.lexema[i++] = c;
                    state = 1; // Identificador ou Palavra Reservada
                } else if (isdigit(c)) {
                    token.lexema[i++] = c;
                    state = 2; // Número
                } else if (c == '{') {
                    state = 3; // Comentário
                } else if (c == ':') {
                    token.lexema[i++] = c;
                    state = 4; // Atribuição ou Dois Pontos
                } else if (c == '<' || c == '>' || c == '!') {
                    token.lexema[i++] = c;
                    state = 5; // Operador Relacional
                } else {
                    token.lexema[i++] = c;
                    state = 6; // Outros símbolos
                }
                break;

            case 1: // Tratando Identificador
                if (isalnum(c) || c == '_') {
                    token.lexema[i++] = c;
                } else {
                    ungetc(c, inputFile);
                    if (!isPalavraReservada(token.lexema)) {
                        token.simbolo = SIDENTIFICADOR;
                    }
                    return;
                }
                break;

            case 2: // Tratando Número
                if (isdigit(c)) {
                    token.lexema[i++] = c;
                } else {
                    ungetc(c, inputFile);
                    token.simbolo = SNUMERO;
                    return;
                }
                break;

            case 3: // Tratando Comentário
                if (c == '}') {
                    state = 0; // Fim do comentário, volta ao estado inicial
                }
                break;

            case 4: // Tratando Atribuição ou :
                if (c == '=') {
                    token.lexema[i++] = c;
                    token.simbolo = SATRIBUICAO;
                } else {
                    ungetc(c, inputFile);
                    token.simbolo = SDOISPONTOS;
                }
                return;

            case 5: // Tratando Operadores Relacionais
                if (c == '=') {
                    token.lexema[i++] = c;
                    if (token.lexema[0] == '<') token.simbolo = SMENORIGUAL;
                    else if (token.lexema[0] == '>') token.simbolo = SMAIORIGUAL;
                    else token.simbolo = SDIFERENTE;
                } else {
                    ungetc(c, inputFile);
                    if (token.lexema[0] == '<') token.simbolo = SMENOR;
                    else if (token.lexema[0] == '>') token.simbolo = SMAIOR;
                    else erro_lexico("Simbolo invalido", token.lexema); // '!' sozinho não é válido
                }
                return;

            case 6: // Outros Símbolos
                switch (token.lexema[0]) {
                    case ';': token.simbolo = SPONTOVIRGULA; break;
                    case '.': token.simbolo = SPONTO; break;
                    case '(': token.simbolo = SABREPARENTESES; break;
                    case ')': token.simbolo = SFECHAPARENTESES; break;
                    case ',': token.simbolo = SVIRGULA; break;
                    case '+': token.simbolo = SMAIS; break;
                    case '-': token.simbolo = SMENOS; break;
                    case '*': token.simbolo = SMULT; break;
                    case '=': token.simbolo = SIGUAL; break;
                    default: erro_lexico("Simbolo nao reconhecido", token.lexema); break;
                }
                ungetc(c, inputFile);
                return;
        }
    }
     if (strlen(token.lexema) > 0 && state == 1) {
         if (!isPalavraReservada(token.lexema)) {
             token.simbolo = SIDENTIFICADOR;
         }
     } else if (strlen(token.lexema) > 0 && state == 2) {
         token.simbolo = SNUMERO;
     }
     else if (strlen(token.lexema) > 0 && state == 6) {
         // Processa os símbolos de um caractere que chegam ao fim do arquivo
         switch (token.lexema[0]) {
            case ';': token.simbolo = SPONTOVIRGULA; break;
            case '.': token.simbolo = SPONTO; break; // <-- A correção para o seu bug
            case '(': token.simbolo = SABREPARENTESES; break;
            case ')': token.simbolo = SFECHAPARENTESES; break;
            case ',': token.simbolo = SVIRGULA; break;
            case '+': token.simbolo = SMAIS; break;
            case '-': token.simbolo = SMENOS; break;
            case '*': token.simbolo = SMULT; break;
            case '=': token.simbolo = SIGUAL; break;
            default: erro_lexico("Simbolo nao reconhecido", token.lexema); break;
         }
        }
}