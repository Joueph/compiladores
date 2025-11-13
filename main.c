// main.c
#include <stdio.h>
#include "parser.h"
#include "globals.h" // Inclui a declaração de inputFile
#include "gerador.h"

int main(int argc, char *argv[]) {

    iniciaGerador("programa.mvd");


    if (argc != 2) {
        printf("Uso: %s <arquivo_fonte.txt>\n", argv[0]);
        return 1;
    }

    // Atribui o ponteiro do arquivo à variável global
    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        perror("Nao foi possivel abrir o arquivo");
        return 1;
    }

    // Chama o analisador, que agora usará a variável global
    analisadorSintatico();

    finalizaGerador();

    fclose(inputFile);
    return 0;
}