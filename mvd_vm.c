#include "mvd_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define MAX_MEM  20000
#define MAX_INST 20000
#define MAX_LABELS 2000
#define MAX_LINE 256

// Códigos de saída para a Web Interface
#define EXIT_CODE_HALT 0
#define EXIT_CODE_INPUT_REQUIRED 10
#define EXIT_CODE_ERROR 1

typedef enum {
    OP_LDC, OP_LDV, OP_STR, OP_ADD, OP_SUB, OP_MULT, OP_DIVI, OP_INV,
    OP_AND, OP_OR, OP_NEG, OP_CME, OP_CMA, OP_CEQ, OP_CDIF, OP_CMEQ, OP_CMAQ,
    OP_JMP, OP_JMPF, OP_NULL, OP_RD, OP_PRN, OP_ALLOC, OP_DALLOC, OP_CALL, OP_RETURN,
    OP_HLT, OP_START, OP_PARA, OP_INVALID
} Op;

typedef struct {
    Op op;
    int has_arg;
    int arg;
    int arg2;
    int arg_is_label;
    char arg_str[32]; // Para o nome da variável no RD
    char arg_label[32];
} Instr;

typedef struct {
    char name[32];
    int addr;
} Label;

// Estado da VM (Estático + Pilha de Contexto)
typedef struct {
    int M[MAX_MEM]; // Memória de Variáveis (Endereçamento Absoluto)
    int S[MAX_MEM]; // Pilha de Operandos e Backup de Recursão
    int s;          // Topo da Pilha
    int pc;         // Program Counter
} VM_State;

// --- Funções Auxiliares ---

static int is_label_tok(const char *t){
    return t && t[0]=='L' && isdigit((unsigned char)t[1]);
}

static Op op_from_str(const char *s){
    if(!s) return OP_INVALID;
    if(!strcmp(s,"LDC"))  return OP_LDC;
    if(!strcmp(s,"LDV"))  return OP_LDV;
    if(!strcmp(s,"STR"))  return OP_STR;
    if(!strcmp(s,"ADD"))  return OP_ADD;
    if(!strcmp(s,"SUB"))  return OP_SUB;
    if(!strcmp(s,"MULT")) return OP_MULT;
    if(!strcmp(s,"DIVI")) return OP_DIVI;
    if(!strcmp(s,"INV"))  return OP_INV;
    if(!strcmp(s,"AND"))  return OP_AND;
    if(!strcmp(s,"OR"))   return OP_OR;
    if(!strcmp(s,"NEG"))  return OP_NEG;
    if(!strcmp(s,"CME"))  return OP_CME;
    if(!strcmp(s,"CMA"))  return OP_CMA;
    if(!strcmp(s,"CEQ"))  return OP_CEQ;
    if(!strcmp(s,"CDIF")) return OP_CDIF;
    if(!strcmp(s,"CMEQ")) return OP_CMEQ;
    if(!strcmp(s,"CMAQ")) return OP_CMAQ;
    if(!strcmp(s,"JMP"))  return OP_JMP;
    if(!strcmp(s,"JMPF")) return OP_JMPF;
    if(!strcmp(s,"NULL")) return OP_NULL;
    if(!strcmp(s,"RD"))   return OP_RD;
    if(!strcmp(s,"PRN"))  return OP_PRN;
    if(!strcmp(s,"ALLOC")) return OP_ALLOC;
    if(!strcmp(s,"DALLOC")) return OP_DALLOC;
    if(!strcmp(s,"CALL"))  return OP_CALL;
    if(!strcmp(s,"RETURN")) return OP_RETURN;
    if(!strcmp(s,"HLT"))    return OP_HLT;
    if(!strcmp(s,"PARA"))   return OP_PARA;
    if(!strcmp(s,"START"))  return OP_START;
    return OP_INVALID;
}

static int find_label(Label *labels, int nlab, const char *name){
    for(int i=0;i<nlab;i++){
        if(!strcmp(labels[i].name,name)) return labels[i].addr;
    }
    return -1;
}

static int load_program(const char *path, Instr *prog, int *nprog_out) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    int nprog = 0;
    Label labels[MAX_LABELS];
    int nlab = 0;
    char line[MAX_LINE];

    while(fgets(line,sizeof(line),f)){
        for(int i=0; line[i]; i++) if(line[i]==';' || line[i]=='#'){ line[i]='\0'; break; }
        
        char *tok[5]; int nt=0;
        char *p = strtok(line," \t\r\n,");
        while(p && nt<5){ tok[nt++]=p; p=strtok(NULL," \t\r\n,"); }
        if(nt==0) continue;

        int idx = 0;
        if(is_label_tok(tok[0])){
            if(nlab < MAX_LABELS){
                strncpy(labels[nlab].name, tok[0], 31);
                labels[nlab].addr = nprog;
                nlab++;
            }
            idx = 1;
            if(idx >= nt) continue; 
        }

        Op op = op_from_str(tok[idx]);
        if(op == OP_INVALID){
            fprintf(stderr,"[MVD] opcode invalido: %s\n", tok[idx]);
            fclose(f); return -1;
        }

        Instr ins; memset(&ins,0,sizeof(ins));
        ins.op = op;

        if(idx+1 < nt){
            ins.has_arg = 1;
            if(is_label_tok(tok[idx+1])){
                ins.arg_is_label = 1;
                strncpy(ins.arg_label, tok[idx+1], 31);
            }else{
                ins.arg = atoi(tok[idx+1]);
            }
            if(idx+2 < nt){
                ins.arg2 = atoi(tok[idx+2]); 
                // Para RD, o nome da variável vem no 2º argumento (ex: RD, nome)
                // Mas o strtok conta opcode como 0. Então: opcode, arg1, arg2.
                // Se for RD, pode não ter arg numérico, ou ter dummy.
                if (op == OP_RD) strncpy(ins.arg_str, tok[idx+2], 31);
            }
        }
        prog[nprog++] = ins;
    }
    fclose(f);

    for(int i=0;i<nprog;i++){
        if(prog[i].has_arg && prog[i].arg_is_label){
            int addr = find_label(labels,nlab,prog[i].arg_label);
            if(addr < 0) return -1;
            prog[i].arg = addr;
        }
    }
    *nprog_out = nprog;
    return 0;
}

// --- Persistência de Estado (Para Web Interface) ---
static bool save_state(const char *path, const VM_State *state) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fwrite(state, sizeof(VM_State), 1, f);
    fclose(f);
    return true;
}

static bool load_state(const char *path, VM_State *state) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fread(state, sizeof(VM_State), 1, f);
    fclose(f);
    return true;
}

// --- Execução ---

int run_vm(const char *prog_path, const char *state_path, bool interactive_mode) {
    Instr prog[MAX_INST];
    int nprog = 0;

    if (load_program(prog_path, prog, &nprog) != 0) return EXIT_CODE_ERROR;

    VM_State vm;
    bool state_loaded = false;

    if (interactive_mode) state_loaded = load_state(state_path, &vm);

    if (!state_loaded) {
        memset(&vm, 0, sizeof(VM_State));
        vm.s = -1;
        vm.pc = 0;
    }

    int *M = vm.M;
    int *S = vm.S;
    int *s = &vm.s;
    int *pc = &vm.pc;

    while (*pc < nprog) {
        Instr in = prog[*pc];
        
        switch(in.op) {
            case OP_LDC: S[++(*s)] = in.arg; (*pc)++; break;
            
            // Endereçamento Absoluto (M)
            case OP_LDV: S[++(*s)] = M[in.arg]; (*pc)++; break;
            case OP_STR: M[in.arg] = S[(*s)--]; (*pc)++; break;
            
            case OP_ADD: S[(*s)-1] += S[*s]; (*s)--; (*pc)++; break;
            case OP_SUB: S[(*s)-1] -= S[*s]; (*s)--; (*pc)++; break;
            case OP_MULT:S[(*s)-1] *= S[*s]; (*s)--; (*pc)++; break;
            case OP_DIVI:S[(*s)-1] /= S[*s]; (*s)--; (*pc)++; break;
            
            case OP_INV: S[*s] = -S[*s]; (*pc)++; break;
            case OP_NEG: S[*s] = 1 - S[*s]; (*pc)++; break;
            case OP_AND: S[(*s)-1] = (S[(*s)-1] && S[*s]); (*s)--; (*pc)++; break;
            case OP_OR:  S[(*s)-1] = (S[(*s)-1] || S[*s]); (*s)--; (*pc)++; break;
            
            case OP_CME: S[(*s)-1] = (S[(*s)-1] < S[*s]); (*s)--; (*pc)++; break;
            case OP_CMA: S[(*s)-1] = (S[(*s)-1] > S[*s]); (*s)--; (*pc)++; break;
            case OP_CEQ: S[(*s)-1] = (S[(*s)-1] == S[*s]); (*s)--; (*pc)++; break;
            case OP_CDIF:S[(*s)-1] = (S[(*s)-1] != S[*s]); (*s)--; (*pc)++; break;
            case OP_CMEQ:S[(*s)-1] = (S[(*s)-1] <= S[*s]); (*s)--; (*pc)++; break;
            case OP_CMAQ:S[(*s)-1] = (S[(*s)-1] >= S[*s]); (*s)--; (*pc)++; break;

            case OP_JMP: *pc = in.arg; break;
            case OP_JMPF: if(!S[(*s)--]) *pc = in.arg; else (*pc)++; break;
            
            case OP_NULL:
            case OP_START: (*pc)++; break;

            case OP_CALL:
                S[++(*s)] = (*pc) + 1;
                *pc = in.arg;
                break;

            case OP_RETURN:
                *pc = S[(*s)--];
                break;

            // --- Recursão Estática ---
            // Salva conteúdo da memória na pilha
            case OP_ALLOC: {
                int inicio = in.arg;
                int tam = in.arg2;
                for(int k=0; k<tam; k++) {
                    S[++(*s)] = M[inicio + k];
                }
                (*pc)++;
                break;
            }
            
            // Restaura conteúdo da pilha para a memória
            case OP_DALLOC: {
                int inicio = in.arg;
                int tam = in.arg2;
                for(int k=tam-1; k>=0; k--) {
                    M[inicio + k] = S[(*s)--];
                }
                (*pc)++;
                break;
            }

            // --- Entrada Interativa ---
            case OP_RD: {
                // Tenta ler. Se falhar, assume que precisa de input via web.
                int val;
                if(scanf("%d", &val) != 1) {
                    if (interactive_mode) {
                        if (in.arg_str[0] != '\0') {
                            printf("INPUT_REQUEST_VAR:%s\n", in.arg_str);
                            fflush(stdout);
                        }
                        save_state(state_path, &vm);
                        return EXIT_CODE_INPUT_REQUIRED;
                    }
                    return EXIT_CODE_ERROR;
                }
                // RD lê para a pilha, mas o próximo comando é STR.
                // No modelo original do professor, RD lê direto pra memória ou pilha?
                // O parser gera RD seguido de STR var.
                // Então RD deve empilhar o valor lido.
                S[++(*s)] = val;
                (*pc)++;
                break;
            }

            case OP_PRN:
                printf("%d\n", S[(*s)--]);
                (*pc)++;
                break;

            case OP_PARA:
            case OP_HLT:
                if(interactive_mode) remove(state_path);
                return EXIT_CODE_HALT;

            default:
                fprintf(stderr,"[MVD] opcode nao tratado (%d)\n", in.op);
                return EXIT_CODE_ERROR;
        }
    }
    if(interactive_mode) remove(state_path);
    return EXIT_CODE_HALT;
}

int mvd_run_file(const char *path) {
    return run_vm(path, NULL, false);
}

int mvd_step_execution(const char *prog_path, const char *state_path) {
    return run_vm(prog_path, state_path, true);
}