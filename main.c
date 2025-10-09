#include <stdio.h>
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Nao foi possivel abrir o arquivo");
        return 1;
    }

    analisadorSintatico(file);

    fclose(file);
    return 0;
}