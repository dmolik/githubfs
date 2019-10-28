# GitHub Fetcher

I got curious over the weekend and wonder what it would be like to query the GitHub API in C using [libCurl](https://curl.haxx.se/libcurl/). I quickly realized I was going to need a JSON parser, so I reached for [libfastjson](https://github.com/rsyslog/libfastjson), which is a fork of json-c brought to us by the rsyslog folks.

## Prerequisites

You're going to need a; Compiler, Make of some sort, libfastjson with headers, and libcurl with it's headers. That's it!

## Building

All you need to do is run:

    make

## Running

The fetcher needs two environment variables `GH_USER`; your github username, and `GH_TOKEN`; a personal access token to your public repos. Execution might look something like this:

    export GH_USER=dmolik
    export GH_TOKEN=<redacted>
    ./fetcher
