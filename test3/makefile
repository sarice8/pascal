
# SSL compiler, built using generic SSL runtime module & scanner

COMPILERS_DIR  = /home/sarice/compilers

# Generic SSL runtime module
SSL_RT_DIR     = ${COMPILERS_DIR}/nssl/ssl_rt

SSL_RT_HEADERS = ${SSL_RT_DIR}/ssl_rt.h \
                 ${SSL_RT_DIR}/ssl_begin.h \
                 ${SSL_RT_DIR}/ssl_end.h

SCHEMA_DIR     = ${COMPILERS_DIR}/ssl_rt/schema

SSL_DIR        = ${COMPILERS_DIR}/nssl


# ---------------------------------------------------------

# Debug objects only required if compile with -DDEBUG

DEBUG_DIR      = ${COMPILERS_DIR}/debug
SSLTOOL_DIR    = ${COMPILERS_DIR}/ssltool

DEBUG_OBJS     = ${DEBUG_DIR}/debug.o
DEBUG_STUBS    = ${DEBUG_DIR}/dbgstub.o
SSLTOOL_OBJS   = ${SSLTOOL_DIR}/ssltool_stubs_new.o \
                 ${SSLTOOL_DIR}/ssltool_ui_new.o 

SSLTOOL_LIB_PATH = -L${GUIDEHOME}/lib -L${OPENWINHOME}/lib 
SSLTOOL_LIBS     = -lguide -lguidexv -lxview -lolgx -lX
 
# ---------------------------------------------------------

# Suggested flags to compile program.c with:
#     -TRACE_RECOVERY   -- trace steps in recovery mode

CFLAGS = 

TEST_OBJS      = test.o \
		 test_schema.o \
		 ${SCHEMA_DIR}/node.o \
                 ${SSL_RT_DIR}/ssl_rt.o \
                 ${SSL_RT_DIR}/ssl_scan.o


all:    test testtool


test:  ${TEST_OBJS}
	cc -o test ${TEST_OBJS} \
		${DEBUG_OBJS} ${DEBUG_STUBS}

#  Integrated graphical debugger
testtool:  ${TEST_OBJS}
	cc -o testtool ${SSLTOOL_LIB_PATH} \
		${TEST_OBJS} \
		${DEBUG_OBJS} ${SSLTOOL_OBJS} ${SSLTOOL_LIBS}

# test.o depends on test.tbl when using C table from "ssl -c"


test.o:  test.c test.h test.tbl ${SSL_RT_HEADERS}
	cc -c test.c ${CFLAGS} -g -I${SSL_RT_DIR} -I${DEBUG_DIR} -I${SCHEMA_DIR}


# Compile schema 

test_schema.o:  test_schema.c
	cc -c test_schema.c -g

test_schema.c test_schema.ssl:  test.schema
	${SCHEMA_DIR}/schema test


# Compile SSL code

test.h:	test.ssl test_schema.ssl ${SSL_DIR}/ssl
	${SSL_DIR}/ssl -l -d -c -r test 
	- rm -f test.h
	- rm -f test.tbl
	- rm -f test.lst
	- rm -f test.dbg
	mv ram_test.h   test.h
	mv ram_test.tbl test.tbl
	mv ram_test.lst test.lst
	mv ram_test.dbg test.dbg


