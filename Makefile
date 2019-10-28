
CFLAGS := -Wall -Wextra -pipe
LIBS   := -lcurl -lfastjson

.PHONY: clean

%.o: %.c
	$(CC) $(CFLAGS) -O -o $@ -c $<

fetcher: main.o
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^

clean:
	rm -rf *.o fetcher
