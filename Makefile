
#  SSL runtime modules

# Needs snapshot of supporting tools
RELEASE      = /home/steve/fun/compilers/release_images

DBG_DIR      = $(RELEASE)/debug/1.2.8

# This is the directory we would export our own build to
OUR_RELEASE_DIR = $(RELEASE)/ssl_rt/1.1

# ---------------------------------------------------------

#CFLAGS = -g
CFLAGS = -O3 -Werror

OBJDIR = ./obj-$(OSTYPE)
BINDIR = ./bin-$(OSTYPE)

SRCS = \
  ssl_rt.c \
  ssl_scan.c \

OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

# gcc -MMD will create these additional .d files listing dependencies
DEP = $(ALL_OBJS:%.o=%.d)

INCLUDE = -I$(DBG_DIR)


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
	cp -p ssl_begin.h $(OUR_RELEASE_DIR)
	cp -p ssl_end.h   $(OUR_RELEASE_DIR)
	cp -p ssl_rt.h    $(OUR_RELEASE_DIR)
	mkdir -p $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	cp -p $(OBJDIR)/ssl_rt.o   $(OUR_RELEASE_DIR)/obj-$(OSTYPE)
	cp -p $(OBJDIR)/ssl_scan.o $(OUR_RELEASE_DIR)/obj-$(OSTYPE)

