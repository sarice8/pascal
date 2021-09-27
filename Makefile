
# Needs a snapshot of supporting tools
RELEASE      = /home/steve/fun/compilers/release_images

SSL_RT_DIR = $(RELEASE)/ssl_rt/1.1

# Debugger integrates with schema, to allow dumping nodes
SCHEMA_DIR = $(RELEASE)/schema/1.3

# This is the directory we would export our own build to
OUR_RELEASE_DIR = $(RELEASE)/debug/1.2.8


# ---------------------------------------------------------

#CFLAGS = -g
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


# Release

release:
	mkdir -p $(OUR_RELEASE_DIR)
	cp -p debug.h $(OUR_RELEASE_DIR)
	mkdir -p $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	cp -p $(OBJDIR)/debug.o $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	cp -p $(OBJDIR)/dbgstub.o $(OUR_RELEASE_DIR)/obj-$(OSTYPE)



