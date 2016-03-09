#!/bin/sh

mkdir -p tmp
tar -C tmp/ -xvzf $1
time -p strusInsert -L error_insert.log -s "path=storage;max_open_files=256;write_buffer_size=512K;block_size=4K" -R resources -m analyzer_wikipedia_search -f 1 -c 50000 -t 3 -x "xml" config/wikipedia.ana tmp/
rm -Rf tmp/
