CC?=	gcc
RM?=	rm -f
BISON?=	bison
FLEX?=	flex
INSTALL?=install
DESTDIR?=/usr/local/bin

MAJ_VER=0
MIN_VER=15

WARNFLAGS= -Wsystem-headers -Wall -W -Wno-unused-parameter \
	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith \
	-Wold-style-definition -Wreturn-type -Wwrite-strings \
	-Wswitch -Wshadow -Wcast-align -Wchar-subscripts \
	-Winline -Wnested-externs

#WARNFLAGS+= -Werror -Wcast-qual -Wunused-parameter

VER_FLAGS= -DMAJ_VER=$(MAJ_VER) -DMIN_VER=$(MIN_VER)

CFLAGS=	$(WARNFLAGS) $(VER_FLAGS) -std=c99 -D_BSD_SOURCE
CFLAGS_DEBUG= -O0 -g3
CFLAGS_OPT=   -O4 -flto
LDFLAGS=
LIBS=	-lm -lgmp -lmpfr

ifeq (${DEBUG}, yes)
  CFLAGS += $(CFLAGS_DEBUG)
else
  CFLAGS += $(CFLAGS_OPT)
endif

OBJS=	calc.tab.o lex.yy.o
OBJS+=	linenoise.o
OBJS+=	num.o ast.o var.o func.o hashtable.o safe_mem.o main.o

all: asccalc

asccalc: $(OBJS)
	$(CC) $(CFLAGS) -o asccalc $^ $(LIBS)

calc.tab.c: calc.y lex.yy.h
	$(BISON) -d $<

calc.tab.h: calc.tab.c
	# created by dep

lex.yy.c: calc.l
	$(FLEX) --header-file=lex.yy.h $<

lex.yy.h: lex.yy.c
	# created by dep

clean:
	$(RM) $(OBJS)
	$(RM) asccalc

realclean: clean
	$(RM) calc.tab.c calc.tab.h
	$(RM) lex.yy.c lex.yy.h
	$(RM) *~

install: asccalc
	$(INSTALL) asccalc $(DESTDIR)
