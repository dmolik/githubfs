
CFLAGS := -Wall -Wextra -pipe -pedantic
LIBS   := -lcurl -lfastjson

default: all

all: fetcher

.PHONY: clean

src/%.o: src/%.c
	$(CC) $(CFLAGS) -O -o $@ -c $<

fetcher: src/main.o src/repo.o
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^

clean:
	rm -rf src/*.o *.o fetcher
