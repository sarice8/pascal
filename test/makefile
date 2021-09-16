
# Testing the ssl_rt runtime module integration kit

COMPILERS_DIR = /home/sarice/compilers
DEBUG_DIR    = ${COMPILERS_DIR}/debug
SSLTOOL_DIR  = ${COMPILERS_DIR}/ssltool
SCHEMA_DIR   = ${COMPILERS_DIR}/schema
SSL_RT_DIR   = ${COMPILERS_DIR}/ssl_rt
SSL_DIR      = ${COMPILERS_DIR}/ssl_rt/ssl

SSL_RT_HEADERS = ${SSL_RT_DIR}/ssl_rt.h \
                 ${SSL_RT_DIR}/ssl_begin.h \
                 ${SSL_RT_DIR}/ssl_end.h

DEBUG_OBJS   = ${DEBUG_DIR}/debug.o
DEBUG_STUBS  = ${DEBUG_DIR}/dbgstub.o
SSLTOOL_OBJS = ${SSLTOOL_DIR}/ssltool_stubs_new.o \
               ${SSLTOOL_DIR}/ssltool_ui_new.o 
SSLTOOL_LIB_PATH = -L${GUIDEHOME}/lib -L${OPENWINHOME}/lib 
SSLTOOL_LIBS     = -lguide -lguidexv -lxview -lolgx -lX
 
# ---------------------------------------------------------

TEST_OBJS        = t.o t_scan.o t_schema.o \
		   ${SSL_RT_DIR}/ssl_rt.o \
		   ${SSL_RT_DIR}/ssl_scan.o \
		   ${SCHEMA_DIR}/node.o

all:    t ttool

t:      ${TEST_OBJS}
	cc -o t ${TEST_OBJS} \
		${DEBUG_OBJS} ${DEBUG_STUBS}

ttool:  ${TEST_OBJS}
	cc -o ttool ${SSLTOOL_LIB_PATH} \
		${TEST_OBJS} \
		${DEBUG_OBJS} ${SSLTOOL_OBJS} ${SSLTOOL_LIBS}

# t.o depends on t.tbl when using C table from "ssl -c"

# Shouldn't really require ssl_rt_headers dependency,
# assuming they don't change.
t.o:    t.c t.h ${SCHEMA_DIR}/node.h t.tbl ${SSL_RT_HEADERS}
	cc -c t.c -g -I${SSL_RT_DIR} -I${DEBUG_DIR} -I${SCHEMA_DIR}

t_scan.o:  t_scan.c t.h
	cc -c t_scan.c -g -I${SSL_RT_DIR} -I${DEBUG_DIR} -I${SCHEMA_DIR}

t_schema.o:  t_schema.c
	cc -c t_schema.c -g

t.h:	t.ssl t_schema.ssl
	${SSL_DIR}/ssl -l -d -c t
	- rm -f t.h
	- rm -f t.tbl
	- rm -f t.lst
	- rm -f t.dbg
	mv ram_t.h t.h
	mv ram_t.tbl t.tbl
	mv ram_t.lst t.lst
	mv ram_t.dbg t.dbg

t_schema.c t_schema.ssl:  t.schema
	${SCHEMA_DIR}/schema t

