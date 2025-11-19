// parser.c
#include "parser.h"
#include "globals.h" // Importa 'token' e 'errosCompilacao'
#include "gerador.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Função para reportar erros sintáticos e recuperar (Panic Mode)
void erro_sintatico(const char* mensagem) {
    errosCompilacao++; // Usa a variável global definida em globals.c
    printf("Erro Sintatico: %s. Ultimo token lido: '%s'\n", mensagem, token.lexema);
    
    // Estratégia de Recuperação: Modo Pânico
    // Consome tokens até encontrar um delimitador seguro (ponto e vírgula, fim, ou ponto)
    while (token.simbolo != SPONTOVIRGULA && 
           token.simbolo != SFIM && 
           token.simbolo != SPONTO &&
           strlen(token.lexema) > 0) {
        getToken();
    }

    // Se parou em um ponto e vírgula, consome-o para reiniciar a análise no próximo comando
    if (token.simbolo == SPONTOVIRGULA) {
        getToken();
    }
}

// Inicia a análise
void analisadorSintatico() {
    errosCompilacao = 0;
    gera("START", -1, -1); // Gera a primeira instrução MVD
    getToken(); // Pega o primeiro token
    analisaPrograma();
    gera("HLT", -1, -1);
    
    if (errosCompilacao == 0) {
        printf("Analises concluidas com sucesso!\n");
    } else {
        printf("Analise finalizada com %d erros.\n", errosCompilacao);
        remove("programa.mvd"); // Remove o arquivo se houver erros
    }
}

// <programa> ::= programa <identificador>; <bloco>.
void analisaPrograma() {
    if (token.simbolo == SPROGRAMA) {
        getToken();
        if (token.simbolo == SIDENTIFICADOR) {
            insere_tabela(token.lexema, "programa", -1);
            getToken();
            if (token.simbolo == SPONTOVIRGULA) {
                getToken();
                analisaBloco();
                if (token.simbolo != SPONTO) {
                    erro_sintatico("Ponto final esperado no fim do programa");
                }

                getToken(); 
                
                if (strlen(token.lexema) > 0) {
                    erro_sintatico("Codigo encontrado apos o ponto final do programa");
                }

            } else {
                erro_sintatico("Ponto e virgula esperado apos o nome do programa");
            }
        } else {
            erro_sintatico("Identificador esperado para o nome do programa");
        }
    } else {
        erro_sintatico("Palavra reservada 'programa' esperada");
    }
}

// <bloco> ::= [<etapa de declaração de variáveis>] [<etapa de declaração de sub-rotinas>] <comandos>
void analisaBloco() {
    int enderecoInicioBloco = enderecoAtual;
    int varsAlocadas = 0;

    if (token.simbolo == SVAR) {
        analisaEtapaVariaveis();
    }

    varsAlocadas = enderecoAtual - enderecoInicioBloco;

    if (varsAlocadas > 0) {
        // Usa enderecoInicioBloco em vez de 0 para suporte a recursão correta
        gera("ALLOC", enderecoInicioBloco, varsAlocadas);
    }

    if (token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        analisaSubrotinas();
    }
    analisaComandos();

    if (varsAlocadas > 0) {
        gera("DALLOC", 0, varsAlocadas);
    }
}

// <etapa de declaração de variáveis> ::= var <declaração de variáveis> ; {<declaração de variáveis>;}
void analisaEtapaVariaveis() {
    if(token.simbolo == SVAR) {
        getToken();
        while(token.simbolo == SIDENTIFICADOR) {
            analisaDeclaracaoVariaveis();
            if(token.simbolo == SPONTOVIRGULA) {
                getToken();
            } else {
                erro_sintatico("Ponto e virgula esperado apos declaracao de variavel");
            }
        }
    }
}

// <declaração de variáveis>::= <identificador> {, <identificador>} : <tipo>
void analisaDeclaracaoVariaveis() {
    while(token.simbolo == SIDENTIFICADOR) {
        if (consulta_duplicidade_escopo(token.lexema)) {
            erro_semantico("Identificador ja declarado neste escopo", token.lexema);
        }
        insere_tabela(token.lexema, "variavel", enderecoAtual); 
        enderecoAtual++;

        getToken();
        if (token.simbolo == SVIRGULA) {
            getToken();
        } else if (token.simbolo == SDOISPONTOS) {
            break; 
        } else {
             erro_sintatico("Virgula ou dois-pontos esperado na declaracao de variavel");
             break; 
        }
    }
    if(token.simbolo == SDOISPONTOS) {
        getToken();
        analisaTipo();
    } else {
        erro_sintatico("Dois-pontos esperado apos identificadores de variaveis");
    }
}

// <tipo> ::= (inteiro | booleano)
void analisaTipo() {
    if(token.simbolo == SINTEIRO) {
        coloca_tipo_tabela("inteiro");
        getToken();
    } else if (token.simbolo == SBOOLEANO) {
        coloca_tipo_tabela("booleano");
        getToken();
    } else {
        erro_sintatico("Tipo 'inteiro' ou 'booleano' esperado");
    }
}

// <etapa de declaração de sub-rotinas>
void analisaSubrotinas() {
    // CORREÇÃO: Pula a execução das definições de sub-rotinas para não executar linearmente
    int rotuloFimSubrotinas = novoRotulo();
    gera("JMP", rotuloFimSubrotinas, -1);

    while(token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        if(token.simbolo == SPROCEDIMENTO) {
            analisaDeclaracaoProcedimento();
        } else {
            analisaDeclaracaoFuncao();
        }
        if(token.simbolo == SPONTOVIRGULA) {
            getToken();
        } else {
            erro_sintatico("Ponto e virgula esperado no final da sub-rotina");
        }
    }
    
    // Define o destino do salto (início dos comandos do bloco atual)
    geraRotulo(rotuloFimSubrotinas);
}

// <declaração de procedimento> ::= procedimento <identificador>; <bloco>
void analisaDeclaracaoProcedimento() {

    int rotuloProc = novoRotulo();

    getToken(); // Consome 'procedimento'
    if(token.simbolo == SIDENTIFICADOR) {
        if (consulta_duplicidade_escopo(token.lexema)) {
            erro_semantico("Identificador ja declarado neste escopo", token.lexema);
        }
        insere_tabela(token.lexema, "procedimento", rotuloProc);
        geraRotulo(rotuloProc); 
        
        entra_escopo(); 
        getToken();
        
        if(token.simbolo == SPONTOVIRGULA) {
            getToken();
            analisaBloco();
        } else {
            erro_sintatico("Ponto e virgula esperado apos nome do procedimento");
        }

        gera ("RETURN", -1, -1);
        
        sai_escopo(); 
    } else {
        erro_sintatico("Identificador esperado para nome de procedimento");
    }
}

// <declaração de função> ::= funcao <identificador>: <tipo>; <bloco>
void analisaDeclaracaoFuncao() {
    int rotuloFunc = -1;
    getToken(); // Consome 'funcao'
    if(token.simbolo == SIDENTIFICADOR) {
        char nomeFuncao[50];
        strcpy(nomeFuncao, token.lexema);
        
        getToken();
        if(token.simbolo == SDOISPONTOS) {
            getToken();
            
            rotuloFunc = novoRotulo();
            char tipoRetorno[20]; 

            if(token.simbolo == SINTEIRO) {
                if (consulta_duplicidade_escopo(nomeFuncao)) {
                     erro_semantico("Identificador ja declarado neste escopo", nomeFuncao);
                }
                insere_tabela(nomeFuncao, "funcao inteiro", rotuloFunc);
                strcpy(tipoRetorno, "inteiro");
            } else if (token.simbolo == SBOOLEANO) {
                if (consulta_duplicidade_escopo(nomeFuncao)) {
                     erro_semantico("Identificador ja declarado neste escopo", nomeFuncao);
                }
                insere_tabela(nomeFuncao, "funcao booleano", rotuloFunc);
                strcpy(tipoRetorno, "booleano");
            } else {
                erro_sintatico("Tipo de retorno 'inteiro' ou 'booleano' esperado para a funcao");
                strcpy(tipoRetorno, "inteiro");
            }

            geraRotulo(rotuloFunc);

            entra_escopo(); 

            // Inserir variável local oculta para retorno da função
            insere_tabela(nomeFuncao, tipoRetorno, enderecoAtual++);

            getToken();
            
            if(token.simbolo == SPONTOVIRGULA) {
                getToken();
                analisaBloco();
            } else {
                erro_sintatico("Ponto e virgula esperado apos tipo de retorno da funcao");
            }

            // Carrega o valor de retorno e gera RETURNF
            int ind = consulta_tabela(nomeFuncao);
            if (ind != -1) {
                gera("LDV", 0, tabelaSimbolos[ind].endereco);
            }
            gera("RETURNF", -1, -1); 

            sai_escopo(); 
        } else {
            erro_sintatico("Dois-pontos esperado apos nome da funcao");
        }
    } else {
        erro_sintatico("Identificador esperado para nome da funcao");
    }
}


// <comandos>::= inicio <comando>{;<comando>}[;] fim
void analisaComandos() {
    if (token.simbolo == SINICIO) {
        getToken();
        analisaComandoSimples();
        while (token.simbolo != SFIM && strlen(token.lexema) > 0) {
            if (token.simbolo == SPONTOVIRGULA) {
                getToken();
                if (token.simbolo != SFIM) { 
                   analisaComandoSimples();
                }
            } else {
                erro_sintatico("Ponto e virgula esperado entre comandos");
            }
        }
        getToken(); // Consome 'fim'
    } else {
        erro_sintatico("'inicio' esperado para o bloco de comandos");
    }
}

// <comando> ::= (<atribuição_chprocedimento> | <comando condicional> | ... )
void analisaComandoSimples() {
    if (token.simbolo == SIDENTIFICADOR) {
        analisaAtribuicaoOuChamadaProcedimento();
    } else if (token.simbolo == SSE) {
        analisaSe();
    } else if (token.simbolo == SENQUANTO) {
        analisaEnquanto();
    } else if (token.simbolo == SLEIA) {
        analisaLeitura();
    } else if (token.simbolo == SESCREVA) {
        analisaEscrita();
    } else if (token.simbolo == SINICIO) {
        analisaComandos();
    } else {
        if (token.simbolo != SFIM && token.simbolo != SPONTOVIRGULA) {
             erro_sintatico("Comando invalido");
        }
    }
}

// <atribuição_chprocedimento>
void analisaAtribuicaoOuChamadaProcedimento(){
    char nomeId[50];
    strcpy(nomeId, token.lexema);

    int indice = consulta_tabela(nomeId);
    if (indice == -1) {
        erro_semantico("Identificador nao declarado", nomeId);
        // Recuperação: consome até ; ou fim
        while (token.simbolo != SPONTOVIRGULA && token.simbolo != SFIM && strlen(token.lexema) > 0) getToken();
        return;
    }
    
    getToken();
    
    if(token.simbolo == SATRIBUICAO){ 

        int enderecoVar = tabelaSimbolos[indice].endereco;
        const char* tipoId = tabelaSimbolos[indice].tipo;
        
        if (strcmp(tipoId, "procedimento") == 0 || strcmp(tipoId, "programa") == 0) {
            erro_semantico("Nao se pode atribuir valor a um procedimento ou programa", nomeId);
        }
        
        int tipoVariavel = get_tipo_simbolo(nomeId); 
        
        getToken();
        int tipoExpressao = analisaExpressao(); 

        if (tipoVariavel != -1 && tipoVariavel != tipoExpressao) {
            erro_semantico("Tipos incompativeis na atribuicao", nomeId);
        }

        gera ("STR", 0, enderecoVar); 

    } else { // Chamada de Procedimento
        if (strcmp(tabelaSimbolos[indice].tipo, "procedimento") != 0) {
            erro_semantico("Chamada de procedimento invalida, identificador nao e um procedimento", nomeId);
        }
        gera("CALL", tabelaSimbolos[indice].endereco, -1);
    }
}

// <comando leitura> ::= leia (<identificador>)
void analisaLeitura() {
    int enderecoVar = -1;
    getToken(); 
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
            int indice = consulta_tabela(token.lexema);
            if (indice == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            } else {
                if (strcmp(tabelaSimbolos[indice].tipo, "inteiro") != 0) {
                    erro_semantico("Comando 'leia' so aceita variaveis do tipo inteiro", token.lexema);
                }
                enderecoVar = tabelaSimbolos[indice].endereco;
            }
            
            getToken();
            if(token.simbolo == SFECHAPARENTESES) {
                gera("RD", -1, -1);
                if (enderecoVar != -1) gera("STR", 0, enderecoVar);
                getToken();
            } else {
                erro_sintatico("Fecha parenteses esperado no comando 'leia'");
            }
        } else {
            erro_sintatico("Identificador esperado no comando 'leia'");
        }
    } else {
        erro_sintatico("Abre parenteses esperado no comando 'leia'");
    }
}

// <comando escrita> ::= escreva (<identificador>)
void analisaEscrita() {
    int enderecoVar = -1;
    getToken(); 
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
            int indice = consulta_tabela(token.lexema);
            if (indice == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            } else {
                 if (strcmp(tabelaSimbolos[indice].tipo, "inteiro") != 0 &&
                     strcmp(tabelaSimbolos[indice].tipo, "funcao inteiro") != 0) {
                    erro_semantico("Comando 'escreva' so aceita variavel ou funcao do tipo inteiro", token.lexema);
                }
                enderecoVar = tabelaSimbolos[indice].endereco;
            }

            getToken();
            if(token.simbolo == SFECHAPARENTESES) {
                if (enderecoVar != -1) gera("LDV", 0, enderecoVar);
                gera("PRN", -1, -1);
                getToken();
            } else {
                erro_sintatico("Fecha parenteses esperado no comando 'escreva'");
            }
        } else {
            erro_sintatico("Identificador esperado no comando 'escreva'");
        }
    } else {
        erro_sintatico("Abre parenteses esperado no comando 'escreva'");
    }
}

// <comando enquanto> ::= enquanto <expressão> faca <comando>
void analisaEnquanto() {
    int rotuloInicio = novoRotulo();
    int rotuloFim = novoRotulo();
    geraRotulo(rotuloInicio); 

    getToken(); 
    
    int tipoExpressao = analisaExpressao();
    if (tipoExpressao != 1) { 
        erro_semantico("Expressao booleana esperada no comando 'enquanto'", "");
    }

    gera("JMPF", 0, rotuloFim);

    if(token.simbolo == SFACA) {
        getToken();
        analisaComandoSimples();
        gera("JMP", 0, rotuloInicio);
        geraRotulo(rotuloFim); 

    } else {
        erro_sintatico("'faca' esperado no comando 'enquanto'");
    }
}

// <comando condicional>::= se <expressão> entao <comando> [senao <comando>]
void analisaSe() {
    int rotuloSenao = -1, rotuloFimSe = -1;

    getToken(); 
    
    int tipoExpressao = analisaExpressao();
    if (tipoExpressao != 1) { 
        erro_semantico("Expressao booleana esperada no comando 'se'", "");
    }

    rotuloSenao = novoRotulo();
    gera ("JMPF", 0, rotuloSenao); 
    
    if(token.simbolo == ENTAO) {
        getToken();
        analisaComandoSimples();
        if(token.simbolo == SSENAO) {
            rotuloFimSe = novoRotulo();
            gera("JMP", 0, rotuloFimSe);
            geraRotulo(rotuloSenao);
            getToken();
            analisaComandoSimples();
            geraRotulo(rotuloFimSe);
        } else {
            geraRotulo(rotuloSenao);
        }
    } else {
        erro_sintatico("'entao' esperado no comando 'se'");
    }
}

// <expressão>::= <expressão simples> [<operador relacional><expressão simples>]
int analisaExpressao() {
    int tipo = analisaExpressaoSimples();
    // CORREÇÃO: Gera as instruções de comparação relacional
    if (token.simbolo == SIGUAL || token.simbolo == SDIFERENTE ||
        token.simbolo == SMENOR || token.simbolo == SMENORIGUAL ||
        token.simbolo == SMAIOR || token.simbolo == SMAIORIGUAL) {
        
        Simbolo op = token.simbolo; // Guarda o operador
        getToken();
        int tipo2 = analisaExpressaoSimples();

        if (tipo != tipo2) {
            erro_semantico("Tipos incompativeis em expressao relacional", "");
        }
        
        switch(op) {
            case SIGUAL: gera("CEQ", -1, -1); break;
            case SDIFERENTE: gera("CDIF", -1, -1); break;
            case SMENOR: gera("CME", -1, -1); break;
            case SMAIOR: gera("CMA", -1, -1); break;
            case SMENORIGUAL: gera("CMEQ", -1, -1); break;
            case SMAIORIGUAL: gera("CMAQ", -1, -1); break;
            default: break;
        }
        return 1; 
    }
    return tipo; 
}

// <expressão simples> ::= [+|-] <termo> {(+|-|ou) <termo> }
int analisaExpressaoSimples() {
    int tipo;
    if(token.simbolo == SMAIS || token.simbolo == SMENOS) {
        Simbolo opUnario = token.simbolo;
        getToken();
        tipo = analisaTermo();
        if (tipo != 0) { 
            erro_semantico("Operador unario '+' ou '-' so pode ser usado com tipo inteiro", "");
        }

        if (opUnario == SMENOS) {
            gera("INV", -1, -1);
        }

    } else {
        tipo = analisaTermo();
    }

    while(token.simbolo == SMAIS || token.simbolo == SMENOS || token.simbolo == SOU) {
        Simbolo op = token.simbolo;
        getToken();
        int tipo2 = analisaTermo();

        if (op == SOU) {
            if (tipo != 1 || tipo2 != 1) {
                erro_semantico("Operador 'ou' so pode ser usado com expressoes booleanas", "");
            }
            tipo = 1; 
            gera ("OR", -1, -1);
        } else { 
             if (tipo != 0 || tipo2 != 0) {
                erro_semantico("Operador '+' ou '-' so pode ser usado com expressoes inteiras", "");
            }
            if (op == SMAIS) gera("ADD", -1, -1);
            else gera("SUB", -1, -1);
            tipo = 0; 
        }
    }
    return tipo;
}

// <termo>::= <fator> {(* | div | e) <fator>}
int analisaTermo() {
    int tipo = analisaFator();
    
    while(token.simbolo == SMULT || token.simbolo == SDIV || token.simbolo == SE) {
        Simbolo op = token.simbolo;
        getToken();
        int tipo2 = analisaFator();
        
        if (op == SE) {
            if (tipo != 1 || tipo2 != 1) {
                erro_semantico("Operador 'e' so pode ser usado com expressoes booleanas", "");
            }
            tipo = 1; 
            gera("AND", -1, -1);
        } else { 
             if (tipo != 0 || tipo2 != 0) {
                erro_semantico("Operador '*' ou 'div' so pode ser usado com expressoes inteiras", "");
            }
            if (op == SMULT) gera ("MULT", -1, -1);
            else gera ("DIVI", -1, -1);
            tipo = 0; 
        }
    }
    return tipo;
}

// <fator> ::= (<variável> | <número> | <chamada de função> | (<expressão>) | verdadeiro | falso | nao <fator>)
int analisaFator() {
    int tipo = -1; 
    
    if (token.simbolo == SIDENTIFICADOR) {
        int indice = consulta_tabela(token.lexema);
        if (indice == -1) {
            erro_semantico("Identificador nao declarado", token.lexema);
            getToken(); // consome o token ruim
            return -1;
        }
        
        const char* tipoId = tabelaSimbolos[indice].tipo;
        if (strcmp(tipoId, "inteiro") == 0) {
            tipo = 0;
            gera("LDV", 0, tabelaSimbolos[indice].endereco);
        } else if (strcmp(tipoId, "booleano") == 0) {
            tipo = 1;
            gera("LDV", 0, tabelaSimbolos[indice].endereco);
        } else if (strcmp(tipoId, "funcao inteiro") == 0) {
            tipo = 0;
            gera("CALL", tabelaSimbolos[indice].endereco, -1);
        } else if (strcmp(tipoId, "funcao booleano") == 0) {
            tipo = 1;
            gera("CALL", tabelaSimbolos[indice].endereco, -1);
        } else {
            erro_semantico("Identificador nao pode ser usado em uma expressao (deve ser var ou funcao)", token.lexema);
        }
        
        getToken();

    } else if (token.simbolo == SNUMERO) {
        tipo = 0; 
        gera("LDC", atoi(token.lexema), -1); 
        getToken();

    } else if (token.simbolo == SNAO) {
        getToken();
        tipo = analisaFator();
        if (tipo != 1) {
            erro_semantico("Operador 'nao' so pode ser usado com tipo booleano", "");
        }
        tipo = 1; 
        gera("NEG", -1, -1); 

    } else if (token.simbolo == SABREPARENTESES) {
        getToken();
        tipo = analisaExpressao();
        if (token.simbolo == SFECHAPARENTESES) {
            getToken();
        } else {
            erro_sintatico("Fecha parenteses esperado na expressao");
        }
    } else if(token.simbolo == SVERDADEIRO || token.simbolo == SFALSO) {
        tipo = 1; 
        if (token.simbolo == SVERDADEIRO) gera("LDC", 1, -1); 
        else gera("LDC", 0, -1); 
        getToken();
    } else {
        erro_sintatico("Fator inesperado na expressao");
        getToken(); // Consome para evitar loop
    }
    
    return tipo;
}