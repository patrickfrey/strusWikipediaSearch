#!/bin/sh

outprefix=origdata/

wget -q -O - http://dumps.wikimedia.your.org/enwiki/20161201/enwiki-20161201-pages-articles.xml.bz2 | bzip2 -d -c | strusWikimediaToXml -f "$outprefix""wikipedia%04u.xml,20M" -n0 -s -
