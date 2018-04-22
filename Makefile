# Makefile for ILOC simulator

CC = clang
CP = clang++

all: opt

opt:	opt.o machine.o instruction.o hash.o lex.yy.o iloc.tab.o
		$(CP) -O2 -o opt opt.o machine.o instruction.o hash.o lex.yy.o iloc.tab.o

opt.o:	opt.cc instruction.h
		$(CP) -O2 -c opt.cc --std=c++11

# sim:	sim.o machine.o instruction.o hash.o lex.yy.o iloc.tab.o
# 		$(CC) -O2 -o sim sim.o machine.o instruction.o hash.o lex.yy.o iloc.tab.o

# sim.o:	sim.c instruction.h machine.h sim.h
# 		$(CC) -O2 -c sim.c

machine.o:	machine.c machine.h
			$(CC) -O2 -c machine.c

instruction.o:	instruction.c instruction.h hash.h
				$(CC) -O2 -c instruction.c

hash.o:	hash.c hash.h
		$(CC) -O2 -c hash.c

lex.yy.o:	lex.yy.c
			$(CC) -O2 -c lex.yy.c

iloc.tab.o:	iloc.tab.c
			$(CC) -O2 -c iloc.tab.c

lex.yy.c:	iloc.l iloc.tab.c instruction.h
			flex iloc.l

iloc.tab.c:	iloc.y instruction.h
			bison -v -t -d iloc.y

clean:
	rm *.o
	rm lex.yy.c
	rm iloc.tab.c
	rm iloc.tab.h
	rm opt

wc:		
		wc -l iloc.y iloc.l hash.h hash.c instruction.h instruction.c machine.h machine.c opt.cc

export:		iloc.y iloc.l hash.c instruction.c machine.c main.c hash.h instruction.h machine.h opt.cc Makefile README
		tar cvf export.tar Makefile README *.c *.h *.l *.y *.cc
