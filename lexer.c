#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Função para reportar erros léxicos e terminarada
void erroLexico(int linha, const char* lexema) {
    printf("Erro lexico na linha %d: caractere ou sequencia invalida '%s'\n", linha, lexema);
    exit(1);
}

// Token global que será usado pelo analisador sintático
Token token;

// Variável global para rastrear o número da linha
int linhaAtual = 1;

// Tabela de palavras reservadas
const char* palavrasReservadas[] = {
    "programa", "inicio", "fim", "var", "inteiro", "booleano",
    "procedimento", "funcao", "se", "enquanto", "faca", "escreva", "leia",
    "verdadeiro", "falso", "ou", "e", "nao", "entao", "senao", "div"
};

Simbolo simbolosReservados[] = {
    SPROGRAMA, SINICIO, SFIM, SVAR, SINTEIRO, SBOOLEANO,
    SPROCEDIMENTO, SFUNCAO, SSE, SENQUANTO, SFACA, SESCREVA, SLEIA,
    SVERDADEIRO, SFALSO, SOU, SE, SNAO, SENTAO, SSENAO, SDIV
};

int isPalavraReservada(const char* lexema) {
    for (int i = 0; i < sizeof(simbolosReservados) / sizeof(Simbolo); i++) {
        if (strcmp(lexema, palavrasReservadas[i]) == 0) {
            token.simbolo = simbolosReservados[i];
            return 1;
        }
    }
    return 0;
}

void getToken(FILE *file) {
    int i = 0;
    char c;
    int state = 0;
    int tokenLinha = 0; // Armazena a linha de início do token

    memset(token.lexema, 0, sizeof(token.lexema));
    token.simbolo = SIMBOLO_ERRO;

    while ((c = fgetc(file)) != EOF) {
        switch (state) {
            case 0: // Estado inicial
                if (c == '\n') {
                    linhaAtual++;
                    continue;
                }
                if (isspace(c)) continue;
                tokenLinha = linhaAtual; // Marca a linha onde o token começa
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
                    ungetc(c, file);
                    if (!isPalavraReservada(token.lexema)) {
                        token.simbolo = SIDENTIFICADOR;
                    }
                    token.linha = tokenLinha;
                    return;
                }
                break;

            case 2: // Tratando Número
                if (isdigit(c)) {
                    token.lexema[i++] = c;
                } else {
                    ungetc(c, file);
                    token.simbolo = SNUMERO;
                    token.linha = tokenLinha;
                    return;
                }
                break;

            case 3: // Tratando Comentário
                if (c == '}') {
                    state = 0; // Fim do comentário, volta ao estado inicial
                } else if (c == '\n') {
                    linhaAtual++;
                }
                break;

            case 4: // Tratando Atribuição ou :
                if (c == '=') {
                    token.lexema[i++] = c;
                    token.simbolo = SATRIBUICAO;
                } else {
                    ungetc(c, file);
                    token.simbolo = SDOISPONTOS;
                }
                token.linha = tokenLinha;
                return;

            case 5: // Tratando Operadores Relacionais
                if (c == '=') {
                    token.lexema[i++] = c;
                    if (token.lexema[0] == '<') token.simbolo = SMENORIGUAL;
                    else if (token.lexema[0] == '>') token.simbolo = SMAIORIGUAL;
                    else token.simbolo = SDIFERENTE;
                } else {
                    ungetc(c, file);
                    if (token.lexema[0] == '<') token.simbolo = SMENOR;
                    else if (token.lexema[0] == '>') token.simbolo = SMAIOR;
                    else {
                        // '!' sozinho não é válido
                        erroLexico(tokenLinha, token.lexema);
                    }
                }
                token.linha = tokenLinha;
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
                    default:
                        erroLexico(tokenLinha, token.lexema);
                        break;
                }
                ungetc(c, file);
                token.linha = tokenLinha;
                return;
        }
    }
     if (strlen(token.lexema) > 0 && state == 1) {
         if (!isPalavraReservada(token.lexema)) {
             token.simbolo = SIDENTIFICADOR;
             token.linha = tokenLinha;
         }
     } else if (strlen(token.lexema) > 0 && state == 2) {
         token.simbolo = SNUMERO;
         token.linha = tokenLinha;
     }

}