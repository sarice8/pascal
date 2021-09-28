
# Needs snapshot of schema compiler version 1.4/boot2 (or current version 1.4) to build this one
RELEASE          = /home/steve/fun/compilers/release_images
SCHEMA_DIR       = ${RELEASE}/schema/1.4

# Generic SSL runtime module
SSL_RT_DIR       = ${RELEASE}/ssl_rt/2.0
SSL_RT_HEADERS   = ${SSL_RT_DIR}/ssl_rt.h \
		   ${SSL_RT_DIR}/ssl_begin.h \
		   ${SSL_RT_DIR}/ssl_end.h

SSL_DIR          = ${RELEASE}/ssl/1.3.2

# This is the directory we would export our own build to
OUR_RELEASE_DIR = $(RELEASE)/schema/1.3

# ---------------------------------------------------------

# Uses Debug version 1.2.8
DBG_DIR          = ${RELEASE}/debug/1.3.0
DBG_OBJS         = ${DBG_DIR}/obj-$(OSTYPE)/debug.o
DBG_STUBS        = ${DBG_DIR}/obj-$(OSTYPE)/dbgstub.o

# Uses SslTool version 1.2.7
# Not available yet, needs porting to a new GUI.
#SSLTOOL_DIR      = ${RELEASE}/ssltool/1.2.7
#SSLTOOL_OBJS     = ${SSLTOOL_DIR}/ssltool_stubs_new.o \
#                   ${SSLTOOL_DIR}/ssltool_ui_new.o
#SSLTOOL_LIB_PATH = -L${GUIDEHOME}/lib -L${OPENWINHOME}/lib
#SSLTOOL_LIBS     = -lguide -lguidexv -lxview -lolgx -lX

# ---------------------------------------------------------

CFLAGS = -g -Werror
#CFLAGS = -O3 -Werror

OBJDIR = ./obj-$(OSTYPE)
BINDIR = ./bin-$(OSTYPE)

SRCS = \
  schema.c \
  schema_schema.c \
  schema_db.c \

BROWSER_SRCS = \
  browser.c \

OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

LINK_OBJS = \
  $(SSL_RT_DIR)/obj-$(OSTYPE)/ssl_rt.o \
  $(SSL_RT_DIR)/obj-$(OSTYPE)/ssl_scan.o \

BROWSER_OBJS = $(BROWSER_SRCS:%.c=$(OBJDIR)/%.o)

ALL_OBJS = $(OBJS) $(LINK_OBJS)

# gcc -MMD will create these additional .d files listing dependencies
DEP = $(ALL_OBJS:%.o=%.d)

INCLUDE = -I$(SSL_RT_DIR) -I$(DBG_DIR)


EXEC = $(BINDIR)/schema

# Main target
all:  $(EXEC) $(BROWSER_OBJS)

# Note:  | $(BINDIR)   means the exec depends on existence of $BINDIR, but its timestamp is irrelevant.
$(EXEC):  $(OBJS) | $(BINDIR)
	cc $^ $(LINK_OBJS) $(DBG_OBJS) $(DBG_STUBS) -o $@


# Tool with integrated graphical debugger

#schema_tool:  ${SCHEMA_OBJS}
#	cc -o schema_tool ${SSLTOOL_LIB_PATH} ${SCHEMA_OBJS} \
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



# Compile schema of schema

schema_schema.c schema_schema.ssl:  schema.schema
	${SCHEMA_DIR}/bin-$(OSTYPE)/schema schema


# Compile SSL code

schema.h: schema.ssl schema_schema.ssl
	$(SSL_DIR)/bin-$(OSTYPE)/ssl -l -d -c schema
	- rm -f schema.h
	- rm -f schema.tbl
	- rm -f schema.lst
	- rm -f schema.dbg
	mv ram_schema.h schema.h
	mv ram_schema.tbl schema.tbl
	mv ram_schema.lst schema.lst
	mv ram_schema.dbg schema.dbg


# Release

release:
	mkdir -p $(OUR_RELEASE_DIR)
	cp -p schema_db.h $(OUR_RELEASE_DIR)
	mkdir -p $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	cp -p $(OBJDIR)/schema_db.o   $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	cp -p $(OBJDIR)/browser.o     $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	mkdir -p $(OUR_RELEASE_DIR)/bin-$(OSTYPE)
	cp -p $(BINDIR)/schema   $(OUR_RELEASE_DIR)/bin-$(OSTYPE)

