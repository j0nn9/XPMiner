VERSION = 1.0.0
SRC     = ./src
BIN     = ./bin
CC      = gcc
DBFLAGS = -g -D DEBUG 
CFLAGS  = -Wall -Wextra -c -Winline -Wformat -Wformat-security \
          -pthread --param max-inline-insns-single=1000 -lm
LDFLAGS = -lcrypto -lgmp -lm -pthread
OTFLAGS = -march=native -flto=6 -O3 -fuse-linker-plugin 

SRC     = src                          
BIN     = bin

.PHONY: clean test all install

# development
#CFLAGS += $(DBFLAGS) 
#CFLAGS += -D CHECK_MULTIPLIER
#CFLAGS += -D CHECK_CANDIDATES
#CFLAGS += -D CHECK_RATIO
#CFLAGS += -D CHECK_SIEVE
#CFLAGS += -D CHECK_PRIMES
#CFLAGS += -D PRINT_TIME
#CFLAGS += -D PRINT_CACHE_TIME
#CFLAGS += -D CHECK_SHARE
#CFLAGS += -D USE_GMP_MILLER_RABIN_TEST

# optimization
CFLAGS  += $(OTFLAGS)
LDFLAGS += $(OTFLAGS)

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
	$(CC) $(LDFLAGS) $(SRC_OBJ) -o $(BIN)/xpminer -lgmp -lcrypto -lm

all: xpminer

install: all
	cp $(BIN)/xpminer /usr/bin/
