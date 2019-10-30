
CFLAGS := -Wall -Wextra -pipe -pedantic
LIBS   := -lcurl -lfastjson -lfuse
DESTDIR :=
PREFIX  := /usr/local


default: all

all: fetcher

.PHONY: clean

BINS := fetcher

src/%.o: src/%.c
	$(CC) $(CFLAGS) -O -o $@ -c $<

fetcher: src/main.o src/repo.o
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^

clean:
	rm -rf src/*.o *.o fetcher

install: $(addprefix install_,$(BINS))

$(addprefix install_,$(BINS)): install_%: %
	install -D -m 0755 $< $(DESTDIR)$(PREFIX)/bin/$<

.PHONY: install $(addprefix install_,$(BINS))
