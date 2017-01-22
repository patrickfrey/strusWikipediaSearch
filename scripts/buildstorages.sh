#!/bin/sh

outprefix=origdata/
blkprefix=strusrepos/
srcprefix=github/strusWikipediaSearch/
threads=12

storageConfig()
{
  echo "path=$blkprefix""storage$1; metadata=redirect UInt8,pageweight UInt8,title_start UInt8,title_end UInt8,doclen UInt32;max_open_files=256;write_buffer_size=512K;block_size=4K"
}

createStorage()
{
  storageid=$1
  strusDestroy -s "`storageConfig $storageid`"
  strusCreate -s "`storageConfig $storageid`"
}

unpackData()
{
  storageid=$1
  docpkglist=$2
  mkdir -p tmp$storageid
  for dd in $docpkglist; do
    tar -C tmp$storageid/ -xvzf "$outprefix"wikipedia$dd.tar.gz
  done
}

buildStorage()
{
  storageid=$1
  time -p strusInsert -s "`storageConfig $storageid`" -R "$srcprefix"config -R "$srcprefix"resources -R "$outprefix"/. -m analyzer_wikipedia_search -m analyzer_pattern -f 1 -c 5000 -t $threads -x "xml" "$srcprefix"config/wikipedia_concepts.ana tmp$storageid/
  rm -Rf tmp$storageid/
}

assignPageweights()
{
  strusUpdateStorage -s "`storageConfig $storageid`" -x titid -m pageweight "$outprefix"pagerank.txt
}

createStorage 1
threads=10
unpackData 1 "00 03 06"
buildStorage 1
threads=6
unpackData 1 "09 12 15"
buildStorage 1

createStorage 2
threads=10
unpackData 2 "01 04 07"
buildStorage 2
threads=6
unpackData 2 "10 13 16"
buildStorage 2


createStorage 3
threads=10
unpackData 3 "02 05 08"
buildStorage 3
threads=6
unpackData 3 "11 14 17"
buildStorage 3

createStorage 4
threads=12
unpackData 4 "18 21 24"
buildStorage 4
threads=8
unpackData 4 "19 22 25"
buildStorage 4

threads=4
unpackData 1 "20"
buildStorage 1
threads=4
unpackData 2 "23"
buildStorage 2
threads=4
unpackData 3 "26"
buildStorage 3


assignPageweights 1
assignPageweights 2
assignPageweights 3
assignPageweights 4

