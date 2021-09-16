# sccsid[] = "%W% %G% %U% %P%";

CC        = cc
CFLAGS    = -c
OPTFLAGS  = -g

INCL      = -I.

all:  debug.o dbgstub.o

debug.o:  debug.c debug.h
	${CC} ${CFLAGS} ${OPTFLAGS} ${INCL} $(@F:.o=.c) -o $@

dbgstub.o:  dbgstub.c
	${CC} ${CFLAGS} ${OPTFLAGS} ${INCL} $(@F:.o=.c) -o $@

