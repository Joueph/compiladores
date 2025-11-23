// mvd_vm.c
#include "mvd_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define MAX_INST 20000
#define MAX_LABELS 2000
#define MAX_MEM  20000
#define MAX_LINE 256

// Códigos de saída especiais
#define EXIT_CODE_HALT 0
#define EXIT_CODE_INPUT_REQUIRED 10
#define EXIT_CODE_ERROR 1

typedef enum {
    OP_LDC, OP_LDV, OP_STR,
    OP_ADD, OP_SUB, OP_MULT, OP_DIVI,
    OP_INV, OP_AND, OP_OR, OP_NEG,
    OP_CME, OP_CMA, OP_CEQ, OP_CDIF, OP_CMEQ, OP_CMAQ,
    OP_JMP, OP_JMPF,
    OP_NULL, OP_RD, OP_PRN,
    OP_PARA,
    OP_START, OP_HLT, OP_ALLOC, OP_DALLOC, OP_CALL, OP_RETURN,
    // --- NOVOS OPCODES PARA ESCOPO E FRAME ---
    OP_NIVEL, OP_LDS, OP_STS, OP_RDS, OP_RETF,
    OP_INVALID
} Op;

typedef struct {
    Op op;
    int has_arg;
    int arg;
    int arg2; 
    int arg_is_label;
    char arg_str[32]; 
    char arg_label[32];
} Instr;

typedef struct {
    char name[32];
    int addr;
} Label;

// Estrutura da VM com Frame Pointer
typedef struct {
    int M[MAX_MEM];
    int s;
    int pc;
    int fp; // <-- NOVO: Aponta para a base do frame atual
} VM_State;


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
    if(!strcmp(s,"PARA")) return OP_PARA;
    if(!strcmp(s,"START"))  return OP_START;
    if(!strcmp(s,"HLT"))    return OP_HLT;
    if(!strcmp(s,"ALLOC"))  return OP_ALLOC;
    if(!strcmp(s,"DALLOC")) return OP_DALLOC;
    if(!strcmp(s,"CALL"))   return OP_CALL;
    if(!strcmp(s,"RETURN")) return OP_RETURN;
    // Novos
    if(!strcmp(s,"NIVEL"))  return OP_NIVEL;
    if(!strcmp(s,"LDS"))    return OP_LDS;
    if(!strcmp(s,"STS"))    return OP_STS;
    if(!strcmp(s,"RDS"))    return OP_RDS;
    if(!strcmp(s,"RETF"))   return OP_RETF;
    
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
    if (!f) { return -1; }

    int nprog = 0;
    Label labels[MAX_LABELS];
    int nlab = 0;
    char line[MAX_LINE];

    while(fgets(line,sizeof(line),f)){
        for(int i=0; line[i]; i++){
            if(line[i]==';' || line[i]=='#'){ line[i]='\0'; break; }
        }
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
                
                // CORREÇÃO: Para RDS, o nome está no índice 3 (Opcode Arg1, Arg2, Nome)
                if (op == OP_RDS && idx+3 < nt) {
                     strncpy(ins.arg_str, tok[idx+3], 31);
                } else if (op == OP_RD) {
                     strncpy(ins.arg_str, tok[idx+2], 31);
                }
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

int run_vm(const char *prog_path, const char *state_path, bool interactive_mode) {
    Instr prog[MAX_INST];
    int nprog = 0;

    if (load_program(prog_path, prog, &nprog) != 0) return EXIT_CODE_ERROR;

    VM_State vm_state;
    bool state_loaded = false;

    if (interactive_mode) state_loaded = load_state(state_path, &vm_state);

    if (!state_loaded) {
        memset(&vm_state, 0, sizeof(VM_State));
        vm_state.s = -1;
        vm_state.pc = 0;
        vm_state.fp = 0; // Inicializa FP
    }

    int *M = vm_state.M;
    int *s_ptr = &vm_state.s;
    int *pc_ptr = &vm_state.pc;
    int *fp_ptr = &vm_state.fp;

    int s = *s_ptr;
    int pc = *pc_ptr;
    int fp = *fp_ptr;
    
    while(pc < nprog){
        Instr in = prog[pc];

        switch(in.op){
        case OP_LDC: M[++s] = in.arg; pc++; break;
        case OP_LDV: M[++s] = M[in.arg]; pc++; break;
        case OP_STR: M[in.arg] = M[s--]; pc++; break;

        case OP_ADD: M[s-1] = M[s-1] + M[s]; s--; pc++; break;
        case OP_SUB: M[s-1] = M[s-1] - M[s]; s--; pc++; break;
        case OP_MULT:M[s-1] = M[s-1] * M[s]; s--; pc++; break;
        case OP_DIVI:M[s-1] = M[s-1] / M[s]; s--; pc++; break;

        case OP_INV: M[s] = -M[s]; pc++; break;
        case OP_NEG: M[s] = 1 - M[s]; pc++; break; 
        case OP_AND: M[s-1] = (M[s-1]==1 && M[s]==1)?1:0; s--; pc++; break;
        case OP_OR:  M[s-1] = (M[s-1]==1 || M[s]==1)?1:0; s--; pc++; break;

        case OP_CME: M[s-1] = (M[s-1] <  M[s])?1:0; s--; pc++; break;
        case OP_CMA: M[s-1] = (M[s-1] >  M[s])?1:0; s--; pc++; break;
        case OP_CEQ: M[s-1] = (M[s-1] == M[s])?1:0; s--; pc++; break;
        case OP_CDIF:M[s-1] = (M[s-1] != M[s])?1:0; s--; pc++; break;
        case OP_CMEQ:M[s-1] = (M[s-1] <= M[s])?1:0; s--; pc++; break;
        case OP_CMAQ:M[s-1] = (M[s-1] >= M[s])?1:0; s--; pc++; break;

        case OP_JMP:  pc = in.arg; break; 
        case OP_JMPF: if(M[s]==0) pc = in.arg; else pc++; s--; break;
        case OP_NULL:
        case OP_START: pc++; break;

        // --- ENTRADA/SAÍDA ---
        case OP_RD: { // Global
            int x;
            if(scanf("%d",&x)!=1){
                if (interactive_mode) {
                    if (in.arg_str[0] != '\0') { printf("INPUT_REQUEST_VAR:%s\n", in.arg_str); fflush(stdout); }
                    *s_ptr = s; *pc_ptr = pc; *fp_ptr = fp;
                    save_state(state_path, &vm_state);
                    return EXIT_CODE_INPUT_REQUIRED;
                }
                return EXIT_CODE_ERROR;
            }
            M[in.arg] = x;
            pc++;
            break;
        }
        case OP_PRN: printf("%d\n", M[s--]); pc++; break;

        // --- STACK FRAME ---

        case OP_CALL:
            M[++s] = pc + 1; // Salva RetAddr
            M[++s] = fp;     // Salva OldFP
            fp = s;          // Novo FP aponta para o OldFP salvo
            pc = in.arg;
            break;

        case OP_RETURN: // Retorno Procedimento
            s = fp;          // Limpa pilha
            fp = M[s--];     // Restaura FP
            pc = M[s--];     // Restaura PC
            break;
        
        case OP_RETF: { // Retorno Função
            // Layout: [RetAddr] [OldFP] [Nivel] [Var0(Ret)] ...
            int val = M[fp + 2]; // Pega valor salvo em 0
            s = fp;
            fp = M[s--];
            pc = M[s--];
            M[++s] = val; // Empilha resultado no chamador
            break;
        }

        case OP_NIVEL:
            M[++s] = in.arg; // Salva Nivel no stack
            pc++;
            break;

        case OP_ALLOC: s += in.arg2; pc++; break;
        case OP_DALLOC: s -= in.arg2; pc++; break;

        // --- ACESSO SCOPED ---

        case OP_LDS: { 
            int nivelAlvo = in.arg2;
            int curr = fp;
            // Caminha pela cadeia estática (OldFP) até achar o nível
            while (curr > 0 && M[curr + 1] != nivelAlvo) curr = M[curr];
            M[++s] = M[curr + 2 + in.arg]; // +2 = Pula OldFP e Nivel
            pc++;
            break;
        }

        case OP_STS: { 
            int nivelAlvo = in.arg2;
            int curr = fp;
            while (curr > 0 && M[curr + 1] != nivelAlvo) curr = M[curr];
            M[curr + 2 + in.arg] = M[s--];
            pc++;
            break;
        }

        case OP_RDS: { 
            int x;
            if(scanf("%d",&x)!=1){
                 if (interactive_mode) {
                    if (in.arg_str[0] != '\0') { printf("INPUT_REQUEST_VAR:%s\n", in.arg_str); fflush(stdout); }
                    *s_ptr = s; *pc_ptr = pc; *fp_ptr = fp;
                    save_state(state_path, &vm_state);
                    return EXIT_CODE_INPUT_REQUIRED;
                }
                return EXIT_CODE_ERROR;
            }
            int nivelAlvo = in.arg2;
            int curr = fp;
            while (curr > 0 && M[curr + 1] != nivelAlvo) curr = M[curr];
            M[curr + 2 + in.arg] = x;
            pc++;
            break;
        }

        case OP_PARA:
        case OP_HLT:
            if (interactive_mode) remove(state_path);
            return EXIT_CODE_HALT;

        default:
            fprintf(stderr,"[MVD] opcode nao tratado (%d)\n", in.op);
            return EXIT_CODE_ERROR;
        }
    }
    if (interactive_mode) remove(state_path);
    return EXIT_CODE_HALT;
}

// -------------------------------------------------------------
// ESTAS SÃO AS FUNÇÕES WRAPPER QUE FALTAVAM E CAUSAVAM O ERRO:
// -------------------------------------------------------------

int mvd_run_file(const char *path) {
    return run_vm(path, NULL, false);
}

int mvd_step_execution(const char *prog_path, const char *state_path) {
    return run_vm(prog_path, state_path, true);
}