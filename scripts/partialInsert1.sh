#!/bin/sh

time bzip2 -c -d data/*.xml.bz2 | strusWikimediaToXml -s -r0G,5G - | strusInsert -c2000 -f1 -n -m analyzer_wikipedia_search -R resources/ "path=storage" config/wikipedia.ana -
echo "0G,5G" inserted into storage 1
