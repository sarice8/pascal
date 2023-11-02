
# Needs a snapshot of supporting tools
RELEASE      = ../release

SSL_RT_DIR = $(RELEASE)/ssl_rt

# Debugger integrates with schema, to allow dumping nodes
SCHEMA_DIR = $(RELEASE)/schema

# This is the directory we would export our own build to
OUR_RELEASE_DIR = $(RELEASE)/debug


# ---------------------------------------------------------

#CFLAGS = -g -Werror
CFLAGS = -O3 -Werror

OBJDIR = ./obj-$(OSTYPE)
BINDIR = ./bin-$(OSTYPE)

SRCS = \
  debug.c \
  dbgstub.c \

OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

# gcc -MMD will create these additional .d files listing dependencies
DEP = $(ALL_OBJS:%.o=%.d)

INCLUDE = -I$(SSL_RT_DIR) -I$(SCHEMA_DIR)


# Main target

all: $(OBJS)


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


clean:
	rm -f $(OBJS)


# Bootstrap: build from existing C/C++ files.  Don't use any other tools.
#
bootstrap:
	make all
	make release


# Release

release_headers:
	mkdir -p $(OUR_RELEASE_DIR)
	cp -p debug.h $(OUR_RELEASE_DIR)

release: release_headers
	mkdir -p $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	cp -p $(OBJDIR)/debug.o $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	cp -p $(OBJDIR)/dbgstub.o $(OUR_RELEASE_DIR)/obj-$(OSTYPE)


