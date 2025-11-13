# O compilador que vamos usar
CC = gcc

# Flags do compilador:
# -Wall = Ativa todos os warnings (muito recomendado)
# -g    = Gera informações de debug (para usar com GDB)
# -std=c99 = Usa o padrão C99
# -MMD -MP = Gera automaticamente os arquivos de dependência (.d)
CFLAGS = -Wall -g -std=c99 -MMD -MP

# O nome do seu programa executável final
# (Baseado no seu .gitignore que menciona 'analisador.exe')
TARGET = analisador

# Lista de todos os seus arquivos-fonte (.c)
SOURCES = main.c lexer.c parser.c simbolo.c globals.c gerador.c

# Gera automaticamente a lista de arquivos-objeto (.o) a partir da lista de fontes
OBJECTS = $(SOURCES:.c=.o)

# Gera automaticamente a lista de arquivos de dependência (.d)
DEPS = $(SOURCES:.c=.d)

# --- Regras do Make ---

# A regra 'all' é a regra padrão (o que o 'make' faz se você não especificar nada)
# Ela diz que queremos construir o nosso TARGET
all: $(TARGET)

# Regra de Linkagem:
# Diz como construir o TARGET (analisador) a partir dos arquivos .o
# Depende de todos os arquivos .o
$(TARGET): $(OBJECTS)
	@echo "Ligando o executável: $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Pronto! Executável '$(TARGET)' criado."

# Regra de Compilação (Implícita):
# O 'make' já sabe como transformar um .c em .o usando $(CC) e $(CFLAGS)
# O -MMD -MP nas CFLAGS cuida automaticamente das dependências dos .h

# Regra 'clean':
# Usada para limpar os arquivos gerados (o executável e os .o)
.PHONY: clean
clean:
	@echo "Limpando arquivos gerados..."
	rm -f $(TARGET) $(OBJECTS) $(DEPS) programa.mvd

# Inclui os arquivos de dependência (.d)
# Isso é o que faz o 'make' ser inteligente e recompilar um .c
# se um .h que ele inclui for modificado.
-include $(DEPS)