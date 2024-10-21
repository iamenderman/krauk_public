COMPILER=gcc
FLAGS= -Wall -g -std=c99 -fsanitize=undefined -lssl -lcrypto

SRCDIR=../shared
LOCAL_SRC = $(wildcard $(SRCDIR)/*.c) 
LOCAL_OBJS = $(patsubst %.c, %.o, $(LOCAL_SRC))
COMPILELINE=$(COMPILER) $(FLAGS) -c $<


main: $(LOCAL_OBJS)
	$(COMPILER) $(FLAGS) % -o $@ $^

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(COMPILELINE)

run: main
	./main
clear:
	-rm *.o main