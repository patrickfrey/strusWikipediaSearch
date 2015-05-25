#!/bin/sh

time bzip2 -c -d /data/*.xml.bz2 | strusWikimediaToXml -s -r0G,30G - | strusInsert -c2000 -f1 -n -m analyzer_wikipedia_search -R resources/ -s "path=/db/strus/wikipedia/storage; max_open_files=256; write_buffer_size=2M" config/wikipedia.ana -
echo "0G,30G" inserted into storage 
