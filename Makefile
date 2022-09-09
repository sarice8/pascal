
# Needs snapshot of supporting tools
RELEASE          = ../release_images

SSL_RT_DIR       = $(RELEASE)/ssl_rt/2.0
SSL_RT_HEADERS   = $(SSL_RT_DIR)/ssl_rt.h \
                   $(SSL_RT_DIR)/ssl_begin.h \
                   $(SSL_RT_DIR)/ssl_end.h

SSL_DIR          = $(RELEASE)/ssl/1.3.2

# ssl_rt wants debug, apparently
DBG_DIR          = $(RELEASE)/debug/1.3.0

DBG_OBJS         = $(DBG_DIR)/obj-$(OSTYPE)/debug.o
DBG_STUBS        = $(DBG_DIR)/obj-$(OSTYPE)/dbgstub.o

# debug wants schema, apparently (even if I'm not using it yet here)
# And note, schema can only link if we have at least an empty schema defined for this application!
SCHEMA_DIR       = $(RELEASE)/schema/1.3



CFLAGS = -g -Werror
#CFLAGS = -O3 -Werror

OBJDIR = ./obj-$(OSTYPE)
BINDIR = ./bin-$(OSTYPE)

FRONT_SRCS = \
  pascal.c \
  pascal_schema.c \

BACK_SRCS = \
  sm.c \

FRONT_OBJS = $(FRONT_SRCS:%.c=$(OBJDIR)/%.o)
BACK_OBJS = $(BACK_SRCS:%.c=$(OBJDIR)/%.o)

FRONT_LINK_OBJS = \
  $(SCHEMA_DIR)/obj-$(OSTYPE)/node.o \
  $(SSL_RT_DIR)/obj-$(OSTYPE)/ssl_rt.o \
  $(SSL_RT_DIR)/obj-$(OSTYPE)/ssl_scan.o \

BACK_LINK_OBJS =


ALL_OBJS = $(FRONT_OBJS) $(FRONT_LINK_OBJS) $(BACK_OBJS) $(BACK_LINK_OBJS)

# gcc -MMD will create .d files listing dependencies
DEP = $(ALL_OBJS:%.o=%.d)

INCLUDE = -I$(SSL_RT_DIR) -I$(DBG_DIR) -I$(SCHEMA_DIR)


FRONT_EXEC = $(BINDIR)/pascal
BACK_EXEC = $(BINDIR)/sm

# Main target

execs: $(FRONT_EXEC) $(BACK_EXEC)

# Note:  | $(BINDIR) means the exec depends on the existence of $BINDIR, but not its timestamp
$(FRONT_EXEC): $(FRONT_OBJS) | $(BINDIR)
	cc $^ $(DBG_OBJS) $(FRONT_LINK_OBJS) $(DBG_STUBS) -o $@

$(BACK_EXEC): $(BACK_OBJS) | $(BINDIR)
	cc $^ $(BACK_LINK_OBJS) -o $@


# Include all .d files
-include $(DEP)

# Compile a c file, and additionally produce a .d dependency file.
$(OBJDIR)/%.o : %.c
	gcc $(CFLAGS) $(INCLUDE) -MMD -c $< -o $@


# Objects depend on the existence of $OBJDIR, but not its timestamp
$(FRONT_OBJS) $(BACK_OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)


# Compile schema
#
# (My toolset using ssl 1.3.2 seems to indirectly require at least an empty schema,
# to resolve some symbols at link time.)
pascal_schema.c pascal_schema.ssl: pascal.schema
	$(SCHEMA_DIR)/bin-$(OSTYPE)/schema pascal


# Compile SSL code
#
pascal.h:  pascal.ssl pascal_schema.ssl
	$(SSL_DIR)/bin-$(OSTYPE)/ssl -l -d -c -r pascal
	- rm -f pascal.h
	- rm -f pascal.tbl
	- rm -f pascal.lst
	- rm -f pascal.dbg
	mv ram_pascal.h   pascal.h
	mv ram_pascal.tbl pascal.tbl
	mv ram_pascal.lst pascal.lst
	mv ram_pascal.dbg pascal.dbg




