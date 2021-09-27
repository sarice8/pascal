
# Needs snapshot of supporting tools
RELEASE          = ../release_images

# This was originally using an early version of ssl, but perhaps a newer version would be ok.
SSL_DIR          = $(RELEASE)/ssl/1.2.8

#CFLAGS = -g -Werror
CFLAGS = -O3 -Werror

OBJDIR = ./obj-$(OSTYPE)
BINDIR = ./bin-$(OSTYPE)

FRONT_SRCS = \
  pascal.c \

BACK_SRCS = \
  sm.c \

FRONT_OBJS = $(FRONT_SRCS:%.c=$(OBJDIR)/%.o)
BACK_OBJS = $(BACK_SRCS:%.c=$(OBJDIR)/%.o)

FRONT_LINK_OBJS =
BACK_LINK_OBJS =


ALL_OBJS = $(FRONT_OBJS) $(FRONT_LINK_OBJS) $(BACK_OBJS) $(BACK_LINK_OBJS)

# gcc -MMD will create .d files listing dependencies
DEP = $(ALL_OBJS:%.o=%.d)

INCLUDE =


FRONT_EXEC = $(BINDIR)/pascal
BACK_EXEC = $(BINDIR)/sm

# Main target

execs: $(FRONT_EXEC) $(BACK_EXEC)

# Note:  | $(BINDIR) means the exec depends on the existence of $BINDIR, but not its timestamp
$(FRONT_EXEC): $(FRONT_OBJS) | $(BINDIR)
	cc $^ $(FRONT_LINK_OBJS) -o $@

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


# To do ... running ssl



