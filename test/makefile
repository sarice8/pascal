
# SSL compiler, built using the SSL compiler that supports variables.

ROOT           = /home/sarice/compilers

# Generic SSL runtime module
SSL_RT_DIR     = ${ROOT}/ssl_rt/2.0
SSL_RT_HEADERS = ${SSL_RT_DIR}/ssl_rt.h \
                 ${SSL_RT_DIR}/ssl_begin.h \
                 ${SSL_RT_DIR}/ssl_end.h

SCHEMA_DIR     = ${ROOT}/schema/1.3

SSL_DIR        = ${ROOT}/ssl/1.3.2


# ---------------------------------------------------------

# Debug objects only required if compile with -DDEBUG

DEBUG_DIR      = ${ROOT}/debug/1.3.0
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

TEST_OBJS      = t.o \
		 t_schema.o \
		 ${SCHEMA_DIR}/node.o \
                 ${SSL_RT_DIR}/ssl_rt.o \
                 ${SSL_RT_DIR}/ssl_scan.o


all:    t ttool


#  Integrated cmdline debugger
t:  ${TEST_OBJS}
	cc -o t ${TEST_OBJS} \
		${DEBUG_OBJS} ${DEBUG_STUBS}

#  Integrated graphical debugger
ttool:  ${TEST_OBJS}
	cc -o ttool ${SSLTOOL_LIB_PATH} \
		${TEST_OBJS} \
		${DEBUG_OBJS} ${SSLTOOL_OBJS} ${SSLTOOL_LIBS}


# Compile for debugger (-DDEBUG):
t.o:  t.c t.h t.tbl ${SSL_RT_HEADERS}
	cc -c t.c ${CFLAGS} -g -I${SSL_RT_DIR} -I${DEBUG_DIR} -I${SCHEMA_DIR}


# Compile schema 

t_schema.o:  t_schema.c
	cc -c t_schema.c -g


# Compile SSL code

t.h:	t.ssl
	${SSL_DIR}/ssl -l -d -c -r t
	- rm -f t.h
	- rm -f t.tbl
	- rm -f t.lst
	- rm -f t.dbg
	mv ram_t.h   t.h
	mv ram_t.tbl t.tbl
	mv ram_t.lst t.lst
	mv ram_t.dbg t.dbg


