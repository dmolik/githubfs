
CFLAGS := -Wall -Wextra -pipe -pedantic
LIBS   := -lcurl -lfastjson

default: all

all: fetcher

.PHONY: clean

%.o: %.c
	$(CC) $(CFLAGS) -O -o $@ -c $<

fetcher: main.o
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^

clean:
	rm -rf *.o fetcher
