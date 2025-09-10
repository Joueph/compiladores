#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Estrutura de tabela de símbolos (palavras reservadas e operadores)
typedef struct {
    char *lexema;
    char *simbolo;
} Token;

// Lista de tokens conhecidos
Token tabela[] = {
    {"programa", "sprograma"},
    {"inicio", "sinicio"},
    {"fim", "sfim"},
    {"procedimento", "sprocedimento"},
    {"funcao", "sfuncao"},
    {"se", "sse"},
    {"entao", "sentao"},
    {"senao", "ssenao"},
    {"enquanto", "senquanto"},
    {"faca", "sfaca"},
    {":=", "satribuicao"},
    {"escreva", "sescreva"},
    {"leia", "sleia"},
    {"var", "svar"},
    {"inteiro", "sinteiro"},
    {"booleano", "sbooleano"},
    {".", "sponto"},
    {";", "sponto_virgula"},
    {",", "svirgula"},
    {"(", "sabre_parenteses"},
    {")", "sfecha_parenteses"},
    {">", "smaior"},
    {">=", "smaiorig"},
    {"=", "sig"},
    {"<", "smenor"},
    {"<=", "smenorig"},
    {"!=", "sdif"},
    {"+", "smais"},
    {"-", "smenos"},
    {"*", "smult"},
    {"div", "sdiv"},
    {"e", "se"},
    {"ou", "sou"},
    {"nao", "snao"},
    {":", "sdoispontos"},
    {"verdadeiro", "sverdadeiro"},
    {"falso", "sfalso"}
};

int tabela_size = sizeof(tabela) / sizeof(Token);

// Função para verificar se o lexema é palavra reservada/operador
char* busca_token(char *lexema) {
    for (int i = 0; i < tabela_size; i++) {
        if (strcmp(tabela[i].lexema, lexema) == 0) {
            return tabela[i].simbolo;
        }
    }
    // Se for número
    int isnum = 1;
    for (int j = 0; lexema[j] != '\0'; j++) {
        if (!isdigit(lexema[j])) {
            isnum = 0;
            break;
        }
    }
    if (isnum) return "snumero";
    // Senão, identificador
    return "sidentificador";
}

// Função principal
int main() {
    FILE *fp = fopen("fonte.txt", "r");
    if (!fp) {
        printf("Erro: não foi possível abrir o arquivo fonte.txt\n");
        return 1;
    }

    char c;
    char buffer[256];
    int idx = 0;
    int in_comment = 0;

    while ((c = fgetc(fp)) != EOF) {
        // Ignorar comentários { ... }
        if (c == '{') {
            in_comment = 1;
            continue;
        }
        if (c == '}') {
            in_comment = 0;
            continue;
        }
        if (in_comment) continue;

        // Separadores
        if (isspace(c) || c == ';' || c == ',' || c == '.' || 
            c == '(' || c == ')' || c == ':' || c == '=' ||
            c == '<' || c == '>' || c == '+' || c == '-' || c == '*') {

            if (idx > 0) {
                buffer[idx] = '\0';
                printf("<%s, %s>\n", buffer, busca_token(buffer));
                idx = 0;
            }

            // Tratar operadores compostos
            if (c == ':' || c == '<' || c == '>' || c == '!') {
                char next = fgetc(fp);
                if ((c == ':' && next == '=') ||
                    (c == '<' && next == '=') ||
                    (c == '>' && next == '=') ||
                    (c == '!' && next == '=')) {
                    char op[3] = {c, next, '\0'};
                    printf("<%s, %s>\n", op, busca_token(op));
                    continue;
                } else {
                    ungetc(next, fp);
                }
            }

            // Operadores e pontuação isolados
            if (!isspace(c)) {
                char op[2] = {c, '\0'};
                printf("<%s, %s>\n", op, busca_token(op));
            }
        } else {
            buffer[idx++] = c;
        }
    }

    // Último buffer
    if (idx > 0) {
        buffer[idx] = '\0';
        printf("<%s, %s>\n", buffer, busca_token(buffer));
    }

    fclose(fp);
    return 0;
}
