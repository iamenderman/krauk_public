COMPILER=gcc
IGNORES= -Wno-pointer-sign -Wno-unknown-pragmas
FLAGS= -Wall -g -std=c99 -fsanitize=undefined -lssl -lcrypto -D_DEFAULT_SOURCE $(IGNORES) 
SRC_DIR=../server
SHARED_DIR=../shared
RES_DIR=./krauk_server_res

SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SHARED_DIR)/*.c) $(wildcard $(RES_DIR)/*.c) 
OBJS = $(patsubst %.c, %.o, $(SRC))

COMPILELINE=$(COMPILER) $(FLAGS) -c $(notdir $<)

main: FLAGS += -DDEBUG
main: $(OBJS)
	$(COMPILER) $(FLAGS) -o server $^

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(COMPILELINE)

$(SHARED_DIR)/%.o: $(SHARED_DIR)/%.c
	cd $(SHARED_DIR); $(COMPILELINE)

$(RES_DIR)/%.o: $(RES_DIR)/%.c
	cd $(RES_DIR); $(COMPILELINE)
run: main
	./server

release: $(OBJS)
	$(COMPILER) $(FLAGS) -o server $^
	
clear:
	-rm *.o server $(SHARED_DIR)/*.o $(RES_DIR)/*.o; rm -r krauk;
valgrind: main
	valgrind ./server

test: clear main
	./server create; ./server track .