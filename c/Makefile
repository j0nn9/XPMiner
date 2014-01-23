VERSION = 1.0.0
SRC   	= ./src
BIN   	= ./bin
CC			= gcc
DBFLAGS	= -g -D DEBUG
CFLAGS	= -Wall -Wextra -c -Werror -Winline -pthread --param max-inline-insns-single=1000
LDFLAGS	= -lcrypto -lgmp -lm -pthread
OTFLAGS = -march=native -O3
SRC			= src
BIN     = bin

.PHONY: clean test

# we are in development
CFLAGS += $(DBFLAGS) 

# optimation
CFLAGS += $(OTFLAGS)

SRC_SRC = $(shell find $(SRC) -type f -name '*.c')
SRC_OBJ = $(SRC_SRC:%.c=%.o)

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

compile: $(SRC_OBJ) $(TEST_OBJ) $(SRC_OBJ)


prepare:
	@mkdir -p bin

link: prepare compile

clean:
	rm -rf $(BIN)
	rm -f $(SRC_OBJ) $(TEST_OBJ) $(SRC_OBJ)

# compile the native binary
xpminer: link
	$(CC) $(LDFLAGS) $(SRC_OBJ) -o $(BIN)/xpminer
