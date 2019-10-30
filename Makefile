
CFLAGS ?= -Wall -Wextra -pipe -pedantic
LIBS   := -lcurl -lfastjson -lfuse
DESTDIR ?=
PREFIX  ?= /usr/local
LDFLAGS := -Wl,--as-needed -L/usr/lib
REPO ?= dmolik

default: all

all: fetcher

.PHONY: clean

BINS := fetcher

src/%.o: src/%.c
	$(CC) $(CFLAGS) -O -o $@ -c $<

fetcher: src/main.o src/repo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf src/*.o *.o fetcher

install: $(addprefix install_,$(BINS))

container: clean
	docker build -t $(REPO)/github_fetcher:latest .

container-run: container
	docker run -it -e GH_USER=${GH_USER} -e GH_TOKEN=${GH_TOKEN} --device /dev/fuse --cap-add SYS_ADMIN --entrypoint sh $(REPO)/github_fetcher:latest

$(addprefix install_,$(BINS)): install_%: %
	install -D -m 0755 $< $(DESTDIR)$(PREFIX)/bin/$<

.PHONY: install $(addprefix install_,$(BINS))
