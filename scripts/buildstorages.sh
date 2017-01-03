#!/bin/sh

outprefix=origdata/
blkrefix=nlpdata/
srcprefix=github/strusWikipediaSearch/
threads=8

storageConfig()
{
	echo "path=$blkrefix""storage$1; metadata=redirect UInt8,title_start UInt8,title_end UInt8,doclen UInt32,pageweight Float32;max_open_files=256;write_buffer_size=512K;block_size=4K"
}

insertDocs()
{
	tarfile=$1
	storageid=$2
	
	mkdir -p tmp$storageid
	tar -C tmp$storageid/ -xvzf $tarfile
	time -p strusInsert -L error_insert$storageid.log -s "`storageConfig $storageid`" -R "$srcprefix"config -R "$srcprefix"resources -R "$outprefix"/. -m analyzer_wikipedia_search -m analyzer_pattern -f 1 -c 5000 -t $threads -x "xml" "$srcprefix"config/wikipedia_concepts.ana tmp$storageid/
	rm -Rf tmp$storageid/
}

buildStorage()
{
	storageid=$1
	docpkglist=$2
	strusDestroy -s "`storageConfig $storageid`"
	strusCreate -s "`storageConfig $storageid`"
	for dd in $docpkglist; do insertDocs "$outprefix"wikipedia$dd.tar.gz $storageid; done
}

buildStorage 1 "00 03 06 09 12 15 18 21 24" &
buildStorage 2 "01 04 07 10 13 16 19 22 25" &
buildStorage 3 "02 05 08 11 14 17 20 23 26" &


