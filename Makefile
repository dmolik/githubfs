
CFLAGS ?= -Wall -Wextra -pipe -pedantic -g
# TODO - add sanatize targets for pipeline -fsanitize=address
LIBS   := -lcurl -lfastjson -lfuse -llmdb
DESTDIR ?=
PREFIX  ?= /usr/local
LDFLAGS := -Wl,--as-needed -L/usr/lib
REPO ?= dmolik


BINS := githubfs


default: all

all: $(BINS)

.PHONY: clean

src/%.o: src/%.c
	$(CC) $(CFLAGS) -O -o $@ -c $<

# TODO - do some fancy gymnastics here and abtract out target
githubfs: src/main.o src/fs.o src/repo.o src/fetch.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf src/*.o *.o $(BINS)

install: $(addprefix install_,$(BINS))

container: clean
	docker build -t $(REPO)/github_fetcher:latest .

container-run: container
	docker run -it -e GH_USER=${GH_USER} -e GH_TOKEN=${GH_TOKEN} --device /dev/fuse --cap-add SYS_ADMIN --entrypoint sh $(REPO)/github_fetcher:latest

$(addprefix install_,$(BINS)): install_%: %
	install -D -m 0755 $< $(DESTDIR)$(PREFIX)/bin/$<

.PHONY: install $(addprefix install_,$(BINS))
