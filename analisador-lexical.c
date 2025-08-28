#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --------------------- Definições ---------------------

#define MAX_LEXEMA 100
#define MAX_TOKENS 1000

typedef enum {
    // Palavras reservadas
    SPROGRAMA, SSE, SENTAO, SSENAO, SENQUANTO, SFACA,
    SINICIO, SFIM, SESCREVA, SLEIA, SVAR, SINTEIRO, SBOLEANO,
    SVERDADEIRO, SFALSO, SPROCEDIMENTO, SFUNCAO, SDIV,
    SE, SOU, SNAO,
    // Outros
    SNUMERO, SIDENTIFICADOR,
    SATRIBUICAO, // :=
    SOMA, SUB, MULT,
    OP_REL, // <, >, =, != etc.
    SPONTUACAO,
    SERRO,
    SFIMARQ
} TipoSimbolo;

typedef struct {
    TipoSimbolo simbolo;
    char lexema[MAX_LEXEMA];
} Token;

// --------------------- Variáveis globais ---------------------

FILE *fonte;
char caractere;
Token listaTokens[MAX_TOKENS];
int qtdTokens = 0;

// --------------------- Funções auxiliares ---------------------

void lerCaractere() {
    int c = fgetc(fonte);
    if (c == EOF) {
        caractere = '\0';
    } else {
        caractere = (char)c;
    }
}

void inserirToken(TipoSimbolo simb, const char *lexema) {
    if (qtdTokens < MAX_TOKENS) {
        listaTokens[qtdTokens].simbolo = simb;
        strcpy(listaTokens[qtdTokens].lexema, lexema);
        qtdTokens++;
    }
}

// --------------------- Tratadores de tokens ---------------------

void trataDigito() {
    char num[MAX_LEXEMA] = "";
    int i = 0;
    while (isdigit(caractere)) {
        num[i++] = caractere;
        lerCaractere();
    }
    num[i] = '\0';
    inserirToken(SNUMERO, num);
}

int ehLetraOuUnderscore(char c) {
    return isalpha(c) || c == '_';
}

void trataIdentificadorOuReservada() {
    char id[MAX_LEXEMA] = "";
    int i = 0;
    while (ehLetraOuUnderscore(caractere) || isdigit(caractere)) {
        id[i++] = caractere;
        lerCaractere();
    }
    id[i] = '\0';

    // Palavras reservadas
    if (strcmp(id, "programa") == 0) inserirToken(SPROGRAMA, id);
    else if (strcmp(id, "se") == 0) inserirToken(SSE, id);
    else if (strcmp(id, "entao") == 0) inserirToken(SENTAO, id);
    else if (strcmp(id, "senao") == 0) inserirToken(SSENAO, id);
    else if (strcmp(id, "enquanto") == 0) inserirToken(SENQUANTO, id);
    else if (strcmp(id, "faca") == 0) inserirToken(SFACA, id);
    else if (strcmp(id, "inicio") == 0) inserirToken(SINICIO, id);
    else if (strcmp(id, "fim") == 0) inserirToken(SFIM, id);
    else if (strcmp(id, "escreva") == 0) inserirToken(SESCREVA, id);
    else if (strcmp(id, "leia") == 0) inserirToken(SLEIA, id);
    else if (strcmp(id, "var") == 0) inserirToken(SVAR, id);
    else if (strcmp(id, "inteiro") == 0) inserirToken(SINTEIRO, id);
    else if (strcmp(id, "booleano") == 0) inserirToken(SBOLEANO, id);
    else if (strcmp(id, "verdadeiro") == 0) inserirToken(SVERDADEIRO, id);
    else if (strcmp(id, "falso") == 0) inserirToken(SFALSO, id);
    else if (strcmp(id, "procedimento") == 0) inserirToken(SPROCEDIMENTO, id);
    else if (strcmp(id, "funcao") == 0) inserirToken(SFUNCAO, id);
    else if (strcmp(id, "div") == 0) inserirToken(SDIV, id);
    else if (strcmp(id, "e") == 0) inserirToken(SE, id);
    else if (strcmp(id, "ou") == 0) inserirToken(SOU, id);
    else if (strcmp(id, "nao") == 0) inserirToken(SNAO, id);
    else inserirToken(SIDENTIFICADOR, id);
}

void trataAtribuicaoOuErro() {
    char lex[MAX_LEXEMA] = ":";
    lerCaractere();
    if (caractere == '=') {
        strcat(lex, "=");
        inserirToken(SATRIBUICAO, lex);
        lerCaractere();
    } else {
        inserirToken(SERRO, lex);
    }
}

void trataOperadorOuPontuacao() {
    char op[3] = "";
    op[0] = caractere;
    op[1] = '\0';

    switch (caractere) {
        case '+': inserirToken(SOMA, op); break;
        case '-': inserirToken(SUB, op); break;
        case '*': inserirToken(MULT, op); break;
        case '<': case '>': case '=': case '!':
            inserirToken(OP_REL, op);
            break;
        case ';': case ',': case '(': case ')': case '.':
            inserirToken(SPONTUACAO, op);
            break;
        default:
            inserirToken(SERRO, op);
    }
    lerCaractere();
}

// --------------------- Pega Token ---------------------

void pegaToken() {
    if (isdigit(caractere)) {
        trataDigito();
    } else if (isalpha(caractere) || caractere == '_') {
        trataIdentificadorOuReservada();
    } else if (caractere == ':') {
        trataAtribuicaoOuErro();
    } else if (strchr("+-*<>!=;,.()", caractere)) {
        trataOperadorOuPontuacao();
    } else if (caractere != '\0') {
        char erro[2] = {caractere, '\0'};
        inserirToken(SERRO, erro);
        lerCaractere();
    }
}

// --------------------- Analisador Léxico ---------------------

void analisadorLexico(const char *nomeArquivo) {
    fonte = fopen(nomeArquivo, "r");
    if (!fonte) {
        printf("Erro ao abrir arquivo %s\n", nomeArquivo);
        exit(1);
    }

    lerCaractere();

    while (caractere != '\0') {
        // Ignorar espaços e comentários
        while ((caractere == '{' || isspace(caractere)) && caractere != '\0') {
            if (caractere == '{') {
                while (caractere != '}' && caractere != '\0') {
                    lerCaractere();
                }
                if (caractere == '}') lerCaractere();
            }
            while (isspace(caractere)) lerCaractere();
        }

        if (caractere != '\0') {
            pegaToken();
        }
    }

    inserirToken(SFIMARQ, "EOF");
    fclose(fonte);
}

// --------------------- Main ---------------------

int main() {
    analisadorLexico("fonte.txt");

    printf("Tokens encontrados:\n");
    for (int i = 0; i < qtdTokens; i++) {
        printf("Token: %d, Lexema: %s\n", listaTokens[i].simbolo, listaTokens[i].lexema);
    }

    return 0;
}
