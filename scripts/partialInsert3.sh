#!/bin/sh

bzip2 -d -c /data/*.xml.bz2 | strusWikimediaToXml -s -r10G,15G - | strusInsert -c500 -f1 -n -m analyzer_wikipedia_search -R /home/pfrey/github/strusWikipediaSearch/resources/ "path=/data/storage3" /home/pfrey/github/strusWikipediaSearch/config/wikipedia.ana -
echo "10G,15G" inserted into storage 3
