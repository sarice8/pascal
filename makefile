
# sccsid: '@(#)makefile	1.2 Steve Rice (C) 6/11/93 14:54:03 /files/home/sim/sarice/compilers/schema/SCCS/s.makefile'

ROOT             = /home/sarice/compilers

# Existing schema compiler used to build this one
SCHEMA_DIR       = ${ROOT}/schema/1.2

# Generic SSL runtime module
SSL_RT_DIR       = ${ROOT}/ssl_rt/1.1
SSL_RT_HEADERS   = ${SSL_RT_DIR}/ssl_rt.h \
		   ${SSL_RT_DIR}/ssl_begin.h \
		   ${SSL_RT_DIR}/ssl_end.h

SSL_DIR          = ${ROOT}/ssl/1.2.8

# ---------------------------------------------------------

DEBUG_DIR        = ${ROOT}/debug/1.2.8
SSLTOOL_DIR      = ${ROOT}/ssltool/1.2.7

DEBUG_OBJS       = ${DEBUG_DIR}/debug.o
DEBUG_STUBS      = ${DEBUG_DIR}/dbgstub.o
SSLTOOL_OBJS     = ${SSLTOOL_DIR}/ssltool_stubs_new.o \
                   ${SSLTOOL_DIR}/ssltool_ui_new.o

SSLTOOL_LIB_PATH = -L${GUIDEHOME}/lib -L${OPENWINHOME}/lib
SSLTOOL_LIBS     = -lguide -lguidexv -lxview -lolgx -lX

# ---------------------------------------------------------

SCHEMA_OBJS      = schema.o schema_scan.o schema_schema.o node.o \
		   ${SSL_RT_DIR}/ssl_rt.o \
		   ${SSL_RT_DIR}/ssl_scan.o

all: schema schema_tool

schema:   ${SCHEMA_OBJS}
	cc -o schema ${SCHEMA_OBJS} ${DEBUG_OBJS} ${DEBUG_STUBS}

schema_tool:  ${SCHEMA_OBJS}
	cc -o schema_tool ${SSLTOOL_LIB_PATH} ${SCHEMA_OBJS} \
		${DEBUG_OBJS} ${SSLTOOL_OBJS} ${SSLTOOL_LIBS}

# schema.o depends on schema.tbl when using "ssl -c"
schema.o:   schema.c schema.h node.h schema.tbl ${SSL_RT_HEADERS}
	cc -c schema.c -g -I${DEBUG_DIR} -I${SSL_RT_DIR}

schema_scan.o:   schema_scan.c schema.h ${SSL_RT_HEADERS}
	cc -c schema_scan.c -g -I${SSL_RT_DIR}

schema_schema.o:  schema_schema.c
	cc -c schema_schema.c -g

node.o:   node.c node.h
	cc -c node.c -g

schema.h: schema.ssl schema_schema.ssl
	${SSL_DIR}/ssl -l -d -c schema
	- rm -f schema.h
	- rm -f schema.tbl
	- rm -f schema.lst
	- rm -f schema.dbg
	mv ram_schema.h schema.h
	mv ram_schema.tbl schema.tbl
	mv ram_schema.lst schema.lst
	mv ram_schema.dbg schema.dbg

ssl: schema.h

schema_schema.c schema_schema.ssl:  schema.schema
	${SCHEMA_DIR}/schema schema

