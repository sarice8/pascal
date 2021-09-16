# sccsid[] = "%W% %G% %U% %P%";

ROOT = /home/sarice/compilers
SSL_RT_DIR = ${ROOT}/ssl_rt/1.1

CC        = cc
CFLAGS    = -c
OPTFLAGS  = -g

INCL      = -I. -I${SSL_RT_DIR}

all:  debug.o dbgstub.o

debug.o:  debug.c debug.h
	${CC} ${CFLAGS} ${OPTFLAGS} ${INCL} $(@F:.o=.c) -o $@

dbgstub.o:  dbgstub.c
	${CC} ${CFLAGS} ${OPTFLAGS} ${INCL} $(@F:.o=.c) -o $@

