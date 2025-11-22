#ifndef MVD_VM_H
#define MVD_VM_H

// Executa um arquivo .mvd do início ao fim. Retorna 0 se ok.
int mvd_run_file(const char *path);

// Executa um passo da VM, continuando de um estado salvo se existir.
int mvd_step_execution(const char *prog_path, const char *state_path);

#endif