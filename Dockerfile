FROM alpine:3.10.3

COPY . /tmp/githubfs

RUN apk update \
	&& apk add --no-cache fuse libcurl libfastjson lmdb \
	&& apk add --no-cache --virtual .build-dependencies \
		make gcc fuse-dev curl-dev libfastjson-dev \
		musl-dev binutils lmdb-dev \
	&& cd /tmp/githubfs \
	&& make CFLAGS="-Wall -Wextra -pipe -g -pedantic" \
	&& make PREFIX=/usr install \
	&& strip /usr/bin/githubfs \
	&& cd / \
	&& apk del .build-dependencies \
	&& rm -rf /var/cache/apk/* \
	&& rm -rf /tmp/githubfs \
	&& mkdir /repos

ENTRYPOINT ["githubfs", "/repos"]
