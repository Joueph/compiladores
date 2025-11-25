// main.c
#include <stdio.h>
#include "parser.h"
#include "globals.h" // Inclui a declaração de inputFile
#include "gerador.h"
#include "mvd_vm.h"

int main(int argc, char *argv[]) {
    errosCompilacao = 0; // Inicializa a flag de erros
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

    iniciaGerador("programa.mvd");

    // Chama o analisador, que agora usará a variável global
    analisadorSintatico();

    finalizaGerador();
    fclose(inputFile);

    // Se a compilação foi bem-sucedida, informa o usuário.
    // A execução da VM agora é um passo separado.
    if(errosCompilacao == 0){
        printf("\nArquivo 'programa.mvd' gerado com sucesso.\n");
        printf("Para executar, use: vm programa.mvd\n");
    }
    
    return 0;
}