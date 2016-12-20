#!/bin/sh

blkrefix=nlpdata/
srcprefix=github/strusWikipediaSearch/

storageConfig()
{
	echo "path=$blkrefix""storage$1; metadata=minpos_title UInt8,maxpos_title UInt8,doclen UInt32,pageweight Float32;max_open_files=256;write_buffer_size=512K;block_size=4K"
}

insertDocs()
{
	tarfile=$1
	storageid=$2
	
	mkdir -p tmp$storageid
	tar -C tmp$storageid/ -xvzf $1
	time -p strusInsert -L error_insert.log -s "`storageConfig $storageid`" -R resources -m analyzer_wikipedia_search -f 1 -c 50000 -t 3 -x "xml" config/wikipedia_concepts.ana tmp$storageid/
	rm -Rf tmp$storageid/
}

buildStorage()
{
	storageid=$1
	docpkglist=$2
	strusCreate -s "`storageConfig $storageid`"
	for dd in $docpkglist; do insertDocs data/wikipedia$dd.tar.gz $storageid; done
}

buildStorage 1 "00 03 06 09 12 15 18 21 24" &
buildStorage 2 "01 04 07 10 13 16 19 22 25" &
buildStorage 3 "02 05 08 11 14 17 20 23 26" &


