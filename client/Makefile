COMPILER=gcc
IGNORES= -Wno-pointer-sign -Wno-unknown-pragmas
FLAGS= -Wall -g -O3 -std=c99 -fsanitize=undefined -lssl -lcrypto -D_DEFAULT_SOURCE $(IGNORES) 
SRCDIR=.
SHARED_DIR=../shared
OPS_DIR=./krauk_client_ops

SRC = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SHARED_DIR)/*.c) $(wildcard $(OPS_DIR)/*.c)
OBJS = $(patsubst %.c, %.o, $(SRC))

COMPILELINE=$(COMPILER) $(FLAGS) -c $(notdir $<)

main: FLAGS += -DDEBUG
main: $(OBJS)
	$(COMPILER) $(FLAGS) -o krauk $^

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(COMPILELINE)

$(SHARED_DIR)/%.o: $(SHARED_DIR)/%.c
	cd $(SHARED_DIR); $(COMPILELINE)

$(OPS_DIR)/%.o: $(OPS_DIR)/%.c
	cd $(OPS_DIR); $(COMPILELINE)


run: main
	./krauk

release: $(OBJS)
	$(COMPILER) $(FLAGS) -o krauk $^
	
clear:
	-rm *.o krauk $(SHARED_DIR)/*.o $(OPS_DIR)/*.o; rm -r .krauk;
valgrind: main
	valgrind ./krauk

test: clear main
	./krauk create; ./krauk track test_track

post: main
	./krauk post;