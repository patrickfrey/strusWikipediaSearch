#!/bin/sh

mkdir -p tmp
tar -C tmp/ -xvzf $1
time strusInsert -s "path=storage;max_open_files=256;write_buffer_size=1M;block_size=4K" -R resources -m analyzer_wikipedia_search -f 1 -c 500 -t 2 -x "xml" config/wikipedia.ana tmp/
rm -Rf tmp/
