
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

#SCHEMA_DIR       = $(RELEASE)/schema/1.3
SCHEMA_DIR       = $(RELEASE)/schema/1.4


# Graphics library built into back end needs SDL2
SDL2_CFLAGS = `sdl2-config --cflags`
SDL2_LIBS = `sdl2-config --libs`


CFLAGS = -g -Werror -DINTEGRATE_SSL_DEBUGGER $(SDL2_CFLAGS)
#CFLAGS = -O3 -Werror -DINTEGRATE_SSL_DEBUGGER $(SDL2_CFLAGS)

OBJDIR = ./obj-$(OSTYPE)
BINDIR = ./bin-$(OSTYPE)

FRONT_SRCS = \
  pascal.cc \
  pascal_schema.c \

FRONT_SSL = \
  pascal.ssl \
  pascal_schema.ssl \
  pascal_unit.ssl \
  pascal_decl.ssl \
  pascal_constexpr.ssl \
  pascal_expr.ssl \
  pascal_stmt.ssl \
  pascal_str.ssl \
  pascal_call.ssl \
  pascal_type.ssl \


BACK_SRCS = \
  sm.cc \

JIT_SRCS = \
  jit.cc \

TCODE_SRCS = \
  tcode.cc \

RUNLIB_SRCS = \
  runlib.cc \


# Temp to do - improve.
# Need multiple substitution steps because I have both cc and c
FRONT_OBJS_1 = $(FRONT_SRCS:%.cc=$(OBJDIR)/%.o)
FRONT_OBJS = $(FRONT_OBJS_1:%.c=$(OBJDIR)/%.o)


BACK_OBJS = $(BACK_SRCS:%.cc=$(OBJDIR)/%.o)
JIT_OBJS = $(JIT_SRCS:%.cc=$(OBJDIR)/%.o)
TCODE_OBJS = $(TCODE_SRCS:%.cc=$(OBJDIR)/%.o)
RUNLIB_OBJS = $(RUNLIB_SRCS:%.cc=$(OBJDIR)/%.o)

FRONT_LINK_OBJS = \
  $(SCHEMA_DIR)/obj-$(OSTYPE)/schema_db.o \
  $(SSL_RT_DIR)/obj-$(OSTYPE)/ssl_rt.o \
  $(SSL_RT_DIR)/obj-$(OSTYPE)/ssl_scan.o \

BACK_LINK_OBJS =

JIT_LINK_OBJS =


ALL_OBJS = $(FRONT_OBJS) $(FRONT_LINK_OBJS) $(BACK_OBJS) $(BACK_LINK_OBJS) \
           $(JIT_OBJS) $(JIT_LINK_OBJS) $(TCODE_OBJS) $(RUNLIB_OBJS)

# gcc -MMD will create .d files listing dependencies
DEP = $(ALL_OBJS:%.o=%.d)

INCLUDE = -I$(SSL_RT_DIR) -I$(DBG_DIR) -I$(SCHEMA_DIR)


FRONT_EXEC = $(BINDIR)/pascal
BACK_EXEC = $(BINDIR)/sm
JIT_EXEC = $(BINDIR)/jit


# Main target

execs: $(FRONT_EXEC) $(BACK_EXEC) $(JIT_EXEC)

# Note:  | $(BINDIR) means the exec depends on the existence of $BINDIR, but not its timestamp
$(FRONT_EXEC): $(FRONT_OBJS) $(TCODE_OBJS) | $(BINDIR)
	g++ $^ $(DBG_OBJS) $(FRONT_LINK_OBJS) $(DBG_STUBS) -o $@

$(BACK_EXEC): $(BACK_OBJS) $(TCODE_OBJS) $(RUNLIB_OBJS) | $(BINDIR)
	g++ $^ $(BACK_LINK_OBJS) $(SDL2_LIBS) -o $@

$(JIT_EXEC): $(JIT_OBJS) $(TCODE_OBJS) $(RUNLIB_OBJS) | $(BINDIR)
	g++ $^ $(JIT_LINK_OBJS) $(SDL2_LIBS) -o $@

# Include all .d files
-include $(DEP)

# Compile a c file, and additionally produce a .d dependency file.
$(OBJDIR)/%.o : %.c
	gcc $(CFLAGS) $(INCLUDE) -MMD -c $< -o $@

# Compile a cc file, and additionally produce a .d dependency file.
$(OBJDIR)/%.o : %.cc
	g++ $(CFLAGS) $(INCLUDE) -MMD -c $< -o $@


# Objects depend on the existence of $OBJDIR, but not its timestamp
$(FRONT_OBJS) $(BACK_OBJS) $(JIT_OBJS) $(TCODE_OBJS) $(RUNLIB_OBJS): | $(OBJDIR)

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
pascal.h:  $(FRONT_SSL)
	$(SSL_DIR)/bin-$(OSTYPE)/ssl -l -d -c -r pascal
	- rm -f pascal.h
	- rm -f pascal.tbl
	- rm -f pascal.lst
	- rm -f pascal.dbg
	mv ram_pascal.h   pascal.h
	mv ram_pascal.tbl pascal.tbl
	mv ram_pascal.lst pascal.lst
	mv ram_pascal.dbg pascal.dbg




