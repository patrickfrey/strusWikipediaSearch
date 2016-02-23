#!/bin/sh

mkdir -p tmp
tar -C tmp/ -xvzf $1
time strusInsert -V -s "path=storage;max_open_files=256;write_buffer_size=256K;block_size=4K" -R resources -m analyzer_wikipedia_search -f 1 -c 20000 -t 3 -x "xml" config/wikipedia.ana tmp/
rm -Rf tmp/
