CC      = gcc
CFLAGS  = -Wall
RM      = rm -f


default: sa818config

all: sa818config

sa818config: sa818.c
	$(CC) $(CFLAGS) -o sa818config sa818.c

clean veryclean:
	$(RM) sa818config