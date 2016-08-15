#!/bin/sh

wget -q -O - http://dumps.wikimedia.your.org/enwiki/20160801/enwiki-20160801-pages-articles.xml.bz2 | bzip2 -d -c | strusWikimediaToXml -f "data/wikipedia%04u.xml,20M" -n0 -s -

