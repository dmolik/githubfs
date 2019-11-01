FROM alpine:3.10.3

COPY . /tmp/github_fetcher

RUN apk update \
	&& apk add --no-cache fuse libcurl libfastjson \
	&& apk add --no-cache --virtual .build-dependencies \
		make gcc fuse-dev curl-dev libfastjson-dev \
		musl-dev binutils \
	&& cd /tmp/github_fetcher \
	&& make CFLAGS="-Wall -Wextra -pipe -g -pedantic" \
	&& make PREFIX=/usr install \
	&& strip /usr/bin/fetcher \
	&& cd / \
	&& apk del .build-dependencies \
	&& rm -rf /var/cache/apk/* \
	&& rm -rf /tmp/github_fetcher \
	&& mkdir /repos

ENTRYPOINT ["fetcher", "-f", "/repos"]
