# Compilateur et options
CC = gcc
CFLAGS = -g -Wall -Wextra -std=c11
LDFLAGS = -ljmap

# Tous les fichiers .c du dossier
SRCS = $(wildcard *.c)
# Chaque .c devient un exécutable sans extension
TARGETS = $(SRCS:.c=)

all: $(TARGETS)

# Règle générale : compile chaque .c en exécutable
%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
