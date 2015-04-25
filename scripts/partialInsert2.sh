#!/bin/sh

bzip2 -d -c /data/*.xml.bz2 | strusWikimediaToXml -s -r5G,10G - | strusInsert -c500 -f1 -n -m analyzer_wikipedia_search -R /home/pfrey/github/strusWikipediaSearch/resources/ "path=/data/storage2" /home/pfrey/github/strusWikipediaSearch/config/wikipedia.ana -
echo "5G,10G" inserted into storage 2
