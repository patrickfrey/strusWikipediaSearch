#!/bin/sh

mkdir -p tmp
tar -C tmp/ -xvzf $1
time strusCheckInsert -s "path=storage" -R resources -m analyzer_wikipedia_search -t 3 -x "xml" config/wikipedia.ana tmp/
rm -Rf tmp/
