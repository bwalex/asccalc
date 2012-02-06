




WARNFLAGS= -Wsystem-headers -Wall -W -Wno-unused-parameter \
	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith \
	-Wold-style-definition -Wreturn-type -Wwrite-strings \
	-Wswitch -Wshadow -Wcast-align -Wchar-subscripts \
	-Winline -Wnested-externs

#WARNFLAGS+= -Werror -Wcast-qual -Wunused-parameter

CC= gcc
CFLAGS= -O0 -g3 $(WARNFLAGS)
LDFLAGS=
LIBS= -lm -lgmp -lmpfr -ledit -ltermcap

all: parser lexer
	$(CC) -c $(CFLAGS) calc.tab.c
	$(CC) -c $(CFLAGS) lex.yy.c
	$(CC) -c $(CFLAGS) num.c
	$(CC) -c $(CFLAGS) ast.c
	$(CC) -c $(CFLAGS) var.c
	$(CC) -c $(CFLAGS) func.c
	$(CC) -c $(CFLAGS) hashtable.c
	$(CC) -c $(CFLAGS) safe_mem.c
	$(CC) -c $(CFLAGS) calc.c

	$(CC) -o asccalc $(CFLAGS) calc.tab.o lex.yy.o num.o ast.o var.o func.o hashtable.o safe_mem.o calc.o $(LIBS)

parser:
	bison -d calc.y

lexer: parser
	flex --header-file=lex.yy.h calc.l 

clean:
	rm -f *.o calc

realclean: clean
	rm -f calc.tab.c calc.tab.h
	rm -f lex.yy.c lex.yy.h
	rm -f *~
