
# SSL compiler, built using generic SSL runtime module & scanner

# Needs snapshot of supporting tools
RELEASE        = ../release

# Generic SSL runtime module
SSL_RT_DIR     = ${RELEASE}/ssl_rt
SSL_RT_HEADERS = ${SSL_RT_DIR}/ssl_rt.h \
                 ${SSL_RT_DIR}/ssl_begin.h \
                 ${SSL_RT_DIR}/ssl_end.h

SCHEMA_DIR     = $(RELEASE)/schema

# This SSL compiler used to build this one ...
SSL_DIR        = ${RELEASE}/ssl
# if want to build with self, for testing:
#SSL_DIR        = .


# This is the directory we would export our own build to
OUR_RELEASE_DIR = $(RELEASE)/ssl


# ---------------------------------------------------------

# Debug objects only required if compile with -DDEBUG

DBG_DIR        = ${RELEASE}/debug

DBG_OBJS       = ${DBG_DIR}/obj-$(OSTYPE)/debug.o
DBG_STUBS      = ${DBG_DIR}/obj-$(OSTYPE)/dbgstub.o

# Uses SslTool
# Not available yet, needs porting to a new GUI.
#SSLTOOL_DIR    = ${ROOT}/ssltool
#SSLTOOL_OBJS   = ${SSLTOOL_DIR}/ssltool_stubs_new.o \
#                 ${SSLTOOL_DIR}/ssltool_ui_new.o 
#SSLTOOL_LIB_PATH = -L${GUIDEHOME}/lib -L${OPENWINHOME}/lib 
#SSLTOOL_LIBS     = -lguide -lguidexv -lxview -lolgx -lX
 
# ---------------------------------------------------------

# Suggested flags to compile ssl.c with:
#     -DDEBUG           -- to integrate with debugger
#     -TRACE_RECOVERY   -- trace steps in recovery mode

CFLAGS = -g -Werror -DDEBUG
#CFLAGS = -O3 -Werror -DDEBUG

OBJDIR = ./obj-$(OSTYPE)
BINDIR = ./bin-$(OSTYPE)

SRCS = \
  ssl.c \
  ssl_schema.c \

OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

LINK_OBJS = \
  $(SCHEMA_DIR)/obj-$(OSTYPE)/schema_db.o \
  $(SSL_RT_DIR)/obj-$(OSTYPE)/ssl_rt.o \
  $(SSL_RT_DIR)/obj-$(OSTYPE)/ssl_scan.o \

ALL_OBJS = $(OBJS) $(LINK_OBJS)

# gcc -MMD will create these additional .d files listing dependencies
DEP = $(ALL_OBJS:%.o=%.d)

INCLUDE = -I$(SSL_RT_DIR) -I$(DBG_DIR) -I$(SCHEMA_DIR)


EXEC = $(BINDIR)/ssl

# Main target
all:  $(EXEC)


# Note:  | $(BINDIR)   means the exec depends on existence of $BINDIR, but its timestamp is irrelevant.
$(EXEC):  $(OBJS) | $(BINDIR)
#	cc $^ $(LINK_OBJS) -o $@
	cc $^ $(DBG_OBJS) $(LINK_OBJS) $(DBG_STUBS) -o $@


# Tool with integrated cmdline debugger
EXEC_DBG = $(BINDIR)/ssldbg

$(EXEC_DBG):  $(OBJS) | $(BINDIR)
	cc $^ $(DBG_OBJS) $(LINK_OBJS) $(DBG_STUBS) -o $@


#  Integrated graphical debugger
#ssltool:  $(OBJS)
#	cc -o ssltool ${SSLTOOL_LIB_PATH} \
#		$(OBJS) \
#		${DBG_OBJS} ${SSLTOOL_OBJS} ${SSLTOOL_LIBS}

# Include all .d files
-include $(DEP)

# Compile a c file, and additionally produce a .d dependency file.
$(OBJDIR)/%.o : %.c
	gcc $(CFLAGS) $(INCLUDE) -MMD -c $< -o $@


# Objects depend on the existence of $OBJDIR, but its timestamp is irrelevant
$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)


# Compile schema
#
ssl_schema.c ssl_schema.ssl:  ssl.schema
	$(SCHEMA_DIR)/bin-$(OSTYPE)/schema ssl


# Compile SSL code
#
ssl.h:	ssl.ssl
	$(SSL_DIR)/bin-$(OSTYPE)/ssl -l -d -c -r ssl 
	- rm -f ssl.h
	- rm -f ssl.tbl
	- rm -f ssl.lst
	- rm -f ssl.dbg
	mv ram_ssl.h   ssl.h
	mv ram_ssl.tbl ssl.tbl
	mv ram_ssl.lst ssl.lst
	mv ram_ssl.dbg ssl.dbg


clean:
	rm -f $(OBJS) $(EXEC)


# Bootstrap: build from existing C/C++ files.  Don't use any other tools.
#
bootstrap:
	touch ssl.h
	touch ssl_schema.c
	touch ssl_schema.ssl
	make all
	make release


# Release

release_headers:
	mkdir -p $(OUR_RELEASE_DIR)

release: release_headers
	mkdir -p $(OUR_RELEASE_DIR)/bin-$(OSTYPE)
	cp -p $(BINDIR)/ssl $(OUR_RELEASE_DIR)/bin-$(OSTYPE)/ssl

