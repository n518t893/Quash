PROGNAME = quash

CC = gcc --std=gnu11
CFLAGS = -Wall -g

CFILELIST = quash.c command.c execute.c parsing/memory_pool.c parsing/parsing_interface.c parsing/parse.tab.c parsing/lex.yy.c
HFILELIST = quash.h command.h execute.h parsing/memory_pool.h parsing/parsing_interface.h parsing/parse.tab.h deque.h 

INCLIST = ./src ./src/parsing


SRCDIR = ./src/
OBJDIR = ./obj/

CFILES = $(patsubst %,$(SRCDIR)%,$(CFILELIST))
HFILES = $(patsubst %,$(SRCDIR)%,$(HFILELIST))
OFILES = $(patsubst %.c,$(OBJDIR)%.o,$(CFILELIST))

RAWC = $(patsubst %.c,%,$(addprefix $(SRCDIR), $(CFILELIST)))
RAWH = $(patsubst %.h,%,$(addprefix $(SRCDIR), $(HFILELIST)))

INCDIRS = $(patsubst %,-I%,$(INCLIST))

OBJINNERDIRS = $(patsubst $(SRCDIR)%,$(OBJDIR)%,$(shell find $(SRCDIR) -type d))


# Build the the quash executable
all: $(OBJINNERDIRS) $(PROGNAME)


# Build the object directories
$(OBJINNERDIRS):
	$(foreach dir, $(OBJINNERDIRS), mkdir -p $(dir);)

# Build the quash program
$(PROGNAME): $(OFILES)
	$(CC) $(CFLAGS) $^ -o $(PROGNAME) 

# Generic build target for all compilation units. NOTE: Changing a
# header requires you to rebuild the entire project
$(OBJDIR)%.o: $(SRCDIR)%.c $(HFILES)
	$(CC) $(CFLAGS) -c $(INCDIRS) -o $@ $< 

%lex.yy.c: %parse.l
	lex -o $@ $<

%.tab.c %.tab.h: %.y
	bison -t --verbose --defines=$(dir $@)parse.tab.h -o $(dir $@)parse.tab.c $<


# Clean build
clean:
	rm -f quash $(OBJS) $(PROGNAME) $(OFILES)
	rm -rf $(OBJDIR)
	-rm -rf src/parsing/parse.tab.c src/parsing/parse.tab.h src/parsing/lex.yy.c
%.c: %.y
%.c: %.l
.PHONY: all clean
