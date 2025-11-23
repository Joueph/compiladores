# O compilador que vamos usar
CC = gcc

# Flags do compilador
CFLAGS = -Wall -g -std=c99 -MMD -MP

# --- Detecção de SO para adicionar .exe no Windows ---
EXE_EXT =
ifeq ($(OS),Windows_NT)
	EXE_EXT = .exe
	RM = del /F /Q
else
	RM = rm -f
endif

# --- COMPILADOR (Analisador) ---
TARGET = analisador$(EXE_EXT)
SOURCES = main.c lexer.c parser.c simbolo.c globals.c gerador.c
OBJECTS = $(SOURCES:.c=.o)

# --- MÁQUINA VIRTUAL (Interpretador) ---
VM_TARGET = vm$(EXE_EXT)
VM_SOURCES = mvd_vm.c vm_main.c
VM_OBJECTS = $(VM_SOURCES:.c=.o)

# Dependências
DEPS = $(SOURCES:.c=.d) $(VM_SOURCES:.c=.d)

# Regra padrão: constrói OS DOIS programas
all: $(TARGET) $(VM_TARGET)

# Regra para linkar o Analisador
$(TARGET): $(OBJECTS)
	@echo "Ligando o analisador: $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Regra para linkar a Máquina Virtual
$(VM_TARGET): $(VM_OBJECTS)
	@echo "Ligando a maquina virtual: $(VM_TARGET)..."
	$(CC) $(CFLAGS) -o $(VM_TARGET) $(VM_OBJECTS)

.PHONY: clean

clean:
	-$(RM) *.o *.d $(TARGET) $(VM_TARGET) *mvd *txt *bin

-include $(DEPS)