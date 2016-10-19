#!/bin/sh

tarfile=$1
jobid=$2

mkdir -p tmp$jobid
tar -C tmp$jobid/ -xvzf $1
for ff in `ls tmp$jobid/*.xml`
do
echo "analyze $ff"
strusAnalyze -M /usr/local/lib/strus/modules -m analyzer_wikipedia_search -R resources/ -D "orig,sent=' .\n',para=' .\n',start=' .\n'" config/wikipedia.ana $ff | iconv -c -f utf-8 -t utf-8 - | perl -C -pe 's/\&[a-z]*\;/ /g' | perl -C -pe 's/nbsp[\;]/ /g' | perl -C -pe 's/[—––_=\/;,()\[\]\{\}\"-\-]/ /g' | perl -C -pe 's/([\!\?\:\.])/ \1 /g' | perl -C -pe 's/[ ]+/ /g' | perl -C -pe 's/[ ][.][ ]/ .\n/g'  >> docs.dump.$jobid.txt
done

rm -Rf tmp$jobid/

