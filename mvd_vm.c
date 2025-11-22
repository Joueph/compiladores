#include "mvd_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INST 20000
#define MAX_LABELS 2000
#define MAX_MEM  20000
#define MAX_LINE 256

typedef enum {
    OP_LDC, OP_LDV, OP_STR,
    OP_ADD, OP_SUB, OP_MULT, OP_DIVI,
    OP_INV, OP_AND, OP_OR, OP_NEG,
    OP_CME, OP_CMA, OP_CEQ, OP_CDIF, OP_CMEQ, OP_CMAQ,
    OP_JMP, OP_JMPF,
    OP_NULL, OP_RD, OP_PRN,
    OP_PARA,
    OP_INVALID
} Op;

typedef struct {
    Op op;
    int has_arg;
    int arg;
    int arg_is_label;
    char arg_label[32];
} Instr;

typedef struct {
    char name[32];
    int addr;
} Label;

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
    return OP_INVALID;
}

static int find_label(Label *labels, int nlab, const char *name){
    for(int i=0;i<nlab;i++){
        if(!strcmp(labels[i].name,name)) return labels[i].addr;
    }
    return -1;
}

int mvd_run_file(const char *path){
    FILE *f = fopen(path,"r");
    if(!f){
        perror("mvd_run_file fopen");
        return 1;
    }

    Instr prog[MAX_INST];
    int nprog = 0;

    Label labels[MAX_LABELS];
    int nlab = 0;

    // ---------- PASSO 1: lê linhas, registra labels e instruções ----------
    char line[MAX_LINE];
    while(fgets(line,sizeof(line),f)){
        // remove comentários simples iniciados por ';' ou '#'
        for(int i=0; line[i]; i++){
            if(line[i]==';' || line[i]=='#'){ line[i]='\0'; break; }
        }

        // tokeniza
        char *tok[4]; int nt=0;
        char *p = strtok(line," \t\r\n");
        while(p && nt<4){ tok[nt++]=p; p=strtok(NULL," \t\r\n"); }
        if(nt==0) continue;

        int idx = 0;

        // rótulo opcional no começo
        if(is_label_tok(tok[0])){
            // guarda label para o endereço atual
            if(nlab < MAX_LABELS){
                strncpy(labels[nlab].name, tok[0], 31);
                labels[nlab].name[31]='\0';
                labels[nlab].addr = nprog;
                nlab++;
            }
            idx = 1;
            if(idx >= nt){
                // linha só com label -> ignora (sem instrução)
                continue;
            }
        }

        Op op = op_from_str(tok[idx]);
        if(op == OP_INVALID){
            fprintf(stderr,"[MVD] opcode inválido: %s\n", tok[idx]);
            fclose(f);
            return 2;
        }

        Instr ins; memset(&ins,0,sizeof(ins));
        ins.op = op;

        if(idx+1 < nt){
            ins.has_arg = 1;
            // arg pode ser número ou label
            if(is_label_tok(tok[idx+1])){
                ins.arg_is_label = 1;
                strncpy(ins.arg_label, tok[idx+1], 31);
                ins.arg_label[31]='\0';
            }else{
                ins.arg = atoi(tok[idx+1]);
            }
        }

        prog[nprog++] = ins;
        if(nprog >= MAX_INST){
            fprintf(stderr,"[MVD] programa grande demais\n");
            fclose(f);
            return 3;
        }
    }
    fclose(f);

    // ---------- PASSO 2: resolve labels ----------
    for(int i=0;i<nprog;i++){
        if(prog[i].has_arg && prog[i].arg_is_label){
            int addr = find_label(labels,nlab,prog[i].arg_label);
            if(addr < 0){
                fprintf(stderr,"[MVD] label não encontrada: %s\n", prog[i].arg_label);
                return 4;
            }
            prog[i].arg = addr;
            prog[i].arg_is_label = 0;
        }
    }

    // ---------- EXECUÇÃO ----------
    int M[MAX_MEM]; 
    for(int i=0;i<MAX_MEM;i++) M[i]=0;

    int s = -1;      // topo da pilha (stack pointer)
    int pc = 0;      // program counter
    
    while(pc < nprog){
        Instr in = prog[pc];

        switch(in.op){
        case OP_LDC: // s:=s+1; M[s]:=k
            M[++s] = in.arg; pc++; break;

        case OP_LDV: // s:=s+1; M[s]:=M[n]
            M[++s] = M[in.arg]; pc++; break;

        case OP_STR: // M[n]:=M[s]; s:=s-1
            M[in.arg] = M[s--]; pc++; break;

        case OP_ADD: M[s-1] = M[s-1] + M[s]; s--; pc++; break;
        case OP_SUB: M[s-1] = M[s-1] - M[s]; s--; pc++; break;
        case OP_MULT:M[s-1] = M[s-1] * M[s]; s--; pc++; break;
        case OP_DIVI:M[s-1] = M[s-1] / M[s]; s--; pc++; break;

        case OP_INV: M[s] = -M[s]; pc++; break;
        case OP_NEG: M[s] = 1 - M[s]; pc++; break; // booleano: negação
        case OP_AND: M[s-1] = (M[s-1]==1 && M[s]==1)?1:0; s--; pc++; break;
        case OP_OR:  M[s-1] = (M[s-1]==1 || M[s]==1)?1:0; s--; pc++; break;

        case OP_CME: M[s-1] = (M[s-1] <  M[s])?1:0; s--; pc++; break;
        case OP_CMA: M[s-1] = (M[s-1] >  M[s])?1:0; s--; pc++; break;
        case OP_CEQ: M[s-1] = (M[s-1] == M[s])?1:0; s--; pc++; break;
        case OP_CDIF:M[s-1] = (M[s-1] != M[s])?1:0; s--; pc++; break;
        case OP_CMEQ:M[s-1] = (M[s-1] <= M[s])?1:0; s--; pc++; break;
        case OP_CMAQ:M[s-1] = (M[s-1] >= M[s])?1:0; s--; pc++; break;

        case OP_JMP:  pc = in.arg; break; // desvia sempre
        case OP_JMPF: // se topo == 0, desvia; depois s:=s-1
            if(M[s]==0) pc = in.arg; else pc++;
            s--;
            break;

        case OP_NULL:
            pc++; break;

        case OP_RD: {
            int x;
            if(scanf("%d",&x)!=1){
                fprintf(stderr,"[MVD] erro de leitura\n");
                return 5;
            }
            M[++s] = x;
            pc++;
            break;
        }

        case OP_PRN:
            printf("%d\n", M[s--]);
            pc++;
            break;

        case OP_PARA:
            return 0;

        default:
            fprintf(stderr,"[MVD] opcode não tratado (%d)\n", in.op);
            return 6;
        }
    }

    return 0;
}
