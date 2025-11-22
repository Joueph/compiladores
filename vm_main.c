// vm_main.c
#include <stdio.h>
#include <string.h>
#include "mvd_vm.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.mvd> [--step <state_file>]\n", argv[0]);
        return 1;
    }

    // Modo interativo (usado pelo servidor web)
    if (argc == 4 && strcmp(argv[2], "--step") == 0) {
        const char *prog_path = argv[1];
        const char *state_path = argv[3];
        return mvd_step_execution(prog_path, state_path);
    }

    // Modo de execução normal (linha de comando)
    return mvd_run_file(argv[1]);
}