
# SSL compiler, built using generic SSL runtime module & scanner

ROOT           = /home/sarice/compilers
SCHEMA_DIR     = ${ROOT}/schema/1.2

# Generic SSL runtime module
SSL_RT_DIR     = ${ROOT}/ssl_rt/1.1
SSL_RT_HEADERS = ${SSL_RT_DIR}/ssl_rt.h \
                 ${SSL_RT_DIR}/ssl_begin.h \
                 ${SSL_RT_DIR}/ssl_end.h

# This SSL compiler used to build this one ...
SSL_DIR        = ${ROOT}/ssl/1.2.8


# ---------------------------------------------------------

# Debug objects only required if compile with -DDEBUG

DEBUG_DIR      = ${ROOT}/debug/1.2.8
SSLTOOL_DIR    = ${ROOT}/ssltool/1.2.7

DEBUG_OBJS     = ${DEBUG_DIR}/debug.o
DEBUG_STUBS    = ${DEBUG_DIR}/dbgstub.o
SSLTOOL_OBJS   = ${SSLTOOL_DIR}/ssltool_stubs_new.o \
                 ${SSLTOOL_DIR}/ssltool_ui_new.o 

SSLTOOL_LIB_PATH = -L${GUIDEHOME}/lib -L${OPENWINHOME}/lib 
SSLTOOL_LIBS     = -lguide -lguidexv -lxview -lolgx -lX
 
# ---------------------------------------------------------

# Suggested flags to compile ssl.c with:
#     -DDEBUG           -- to integrate with debugger
#     -TRACE_RECOVERY   -- trace steps in recovery mode

CFLAGS = -DDEBUG

SSL_OBJS       = ssl.o \
                 ${SSL_RT_DIR}/ssl_rt.o \
                 ${SSL_RT_DIR}/ssl_scan.o


all:    ssl ssltool


#  Cmdline ssl without debugger
#ssl:    ${SSL_OBJS}
#	cc -o ssl ${SSL_OBJS}
#		${DEBUG_OBJS} ${DEBUG_STUBS}

#  Integrated cmdline debugger, if ssl.o compiled with -DDEBUG
ssl:  ${SSL_OBJS}
	cc -o ssl ${SSL_OBJS} \
		${DEBUG_OBJS} ${DEBUG_STUBS}

#  Integrated graphical debugger, if ssl.o compiled with -DDEBUG
ssltool:  ${SSL_OBJS}
	cc -o ssltool ${SSLTOOL_LIB_PATH} \
		${SSL_OBJS} \
		${DEBUG_OBJS} ${SSLTOOL_OBJS} ${SSLTOOL_LIBS}

# ssl.o depends on ssl.tbl when using C table from "ssl -c"

# Shouldn't really require ssl_rt_headers dependency,
# assuming they don't change.
#ssl.o:  ssl.c ssl.h ssl.tbl ${SSL_RT_HEADERS}
#	cc -c ssl.c -g -I${SSL_RT_DIR}


# Compile for debugger (-DDEBUG):
ssl.o:  ssl.c ssl.h ssl.tbl ${SSL_RT_HEADERS}
	cc -c ssl.c ${CFLAGS} -g -I${SSL_RT_DIR} -I${DEBUG_DIR}


ssl.h:	ssl.ssl
	${SSL_DIR}/ssl -l -d -c ssl 
	- rm -f ssl.h
	- rm -f ssl.tbl
	- rm -f ssl.lst
	- rm -f ssl.dbg
	mv ram_ssl.h   ssl.h
	mv ram_ssl.tbl ssl.tbl
	mv ram_ssl.lst ssl.lst
	mv ram_ssl.dbg ssl.dbg


