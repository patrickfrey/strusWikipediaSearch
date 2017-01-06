#!/bin/sh

outprefix=origdata/
blkrefix=nlpdata/
srcprefix=github/strusWikipediaSearch/
threads=15

storageConfig()
{
  echo "path=$blkrefix""storage$1; metadata=redirect UInt8,title_start UInt8,title_end UInt8,doclen UInt32,pageweight Float32;max_open_files=256;write_buffer_size=512K;block_size=4K"
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
    tar -C tmp$storageid/ -xvzf $dd
  done
}

buildStorage()
{
  storageid=$1
  time -p strusInsert -s "`storageConfig $storageid`" -R "$srcprefix"config -R "$srcprefix"resources -R "$outprefix"/. -m analyzer_wikipedia_search -m analyzer_pattern -f 1 -c 5000 -t $threads -x "xml" "$srcprefix"config/wikipedia_concepts.ana tmp$storageid/
  rm -Rf tmp$storageid/
}

createStorage 1
unpackData 1 "00 03 06 09"
buildStorage 1
unpackData 1 "12 15 18 21 24"
buildStorage 1

createStorage 2
unpackData 2 "01 04 07 10 13"
buildStorage 2
unpackData 2 "16 19 22 25"
buildStorage 2

createStorage 3
unpackData 3 "02 05 08 11"
buildStorage 3
unpackData 3 "14 17 20 23 26"
buildStorage 3





