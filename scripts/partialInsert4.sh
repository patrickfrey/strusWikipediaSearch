#!/bin/sh

bzip2 -d -c /data/*.xml.bz2 | strusWikimediaToXml -s -r15G,20G - | strusInsert -c500 -f1 -n -m analyzer_wikipedia_search -R /home/pfrey/github/strusWikipediaSearch/resources/ "path=/data/storage4" /home/pfrey/github/strusWikipediaSearch/config/wikipedia.ana -
echo "15G,20G" inserted into storage 4

